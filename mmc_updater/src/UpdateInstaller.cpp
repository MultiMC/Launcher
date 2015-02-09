#include "UpdateInstaller.h"

#include "AppInfo.h"
#include "FileUtils.h"
#include "Log.h"
#include "ProcessUtils.h"
#include "UpdateObserver.h"

void UpdateInstaller::setWaitPid(Q_PID pid)
{
	m_waitPid = pid;
}
void UpdateInstaller::setInstallDir(const QString &path)
{
	m_installDir = path;
}
void UpdateInstaller::setPackageDir(const QString &path)
{
	m_packageDir = path;
}
void UpdateInstaller::setBackupDir(const QString &path)
{
	m_backupDir = path;
}
void UpdateInstaller::setScript(UpdateScript *script)
{
	m_script = script;
}
void UpdateInstaller::setFinishCmd(const QString &cmd)
{
	m_finishCmd = cmd;
}
void UpdateInstaller::setFinishDir(const QString &dir)
{
	m_finishDir = dir;
}
void UpdateInstaller::setObserver(UpdateObserver *observer)
{
	m_observer = observer;
}
void UpdateInstaller::setDryRun(bool dryRun)
{
	m_dryRun = dryRun;
}

void UpdateInstaller::reportError(const QString &error)
{
	if (m_observer)
	{
		m_observer->updateError(error.toStdString());
		m_observer->updateFinished();
	}
	m_wasFailure = true;
}
QString UpdateInstaller::friendlyErrorForError(const FileUtils::IOException &exception) const
{
	QString friendlyError;

	switch (exception.type())
	{
	case FileUtils::IOException::ReadOnlyFileSystem:
#ifdef Q_OS_OSX
		friendlyError = QString::fromStdString(AppInfo::appName()) +
						" was started from a read-only location.  "
						"Copy it to the Applications folder on your Mac and run "
						"it from there.";
#else
		friendlyError =
			QString::fromStdString(AppInfo::appName()) +
			" was started from a read-only location.  "
			"Re-install it to a location that can be updated and run it from there.";
#endif
		break;
	case FileUtils::IOException::DiskFull:
		friendlyError = "The disk is full.  Please free up some space and try again.";
		break;
	default:
		break;
	}

	return friendlyError;
}

void UpdateInstaller::run() throw()
{
	if (!m_script || !m_script->isValid())
	{
		reportError("Unable to read update script");
		return;
	}
	if (m_installDir.isEmpty())
	{
		reportError("No installation directory specified");
		return;
	}

	if (m_waitPid != 0)
	{
		LOG(Info, "Waiting for main app process to finish");
		ProcessUtils::waitForProcess(m_waitPid);
	}

	LOG(Info, "Starting update installation");

	// the detailed error string returned by the OS
	QString error;
	// the message to present to the user.  This may be the same
	// as 'error' or may be different if a more helpful suggestion
	// can be made for a particular problem
	QString friendlyError;

	try
	{
		LOG(Info, "Installing new and updated files");
		installFiles();

		LOG(Info, "Uninstalling removed files");
		uninstallFiles();

		LOG(Info, "Removing backups");
		removeBackups();

		postInstallUpdate();

		LOG(Info, "Update install completed");
	}
	catch (const FileUtils::IOException &exception)
	{
		error = exception.what();
		friendlyError = friendlyErrorForError(exception);
	}
	catch (const QString &genericError)
	{
		error = genericError;
	}

	if (!error.isEmpty())
	{
		LOG(Error, QString("Error installing update ") + error);

		try
		{
			revert();
		}
		catch (const FileUtils::IOException &exception)
		{
			LOG(Error, "Error reverting partial update " + QString(exception.what()));
		}

		if (m_observer)
		{
			if (friendlyError.isEmpty())
			{
				friendlyError = error;
			}
			m_observer->updateError(friendlyError.toStdString());
			m_wasFailure = true;
		}
	}

	if (m_observer)
	{
		m_observer->updateFinished();
	}

	// restart the main application - this is currently done
	// regardless of whether the installation succeeds or not
	restartMainApp();

	// clean up files created by the updater
	cleanup();
}

void UpdateInstaller::cleanup()
{
	try
	{
		FileUtils::removeDir(m_packageDir);
	}
	catch (const FileUtils::IOException &ex)
	{
		LOG(Error, "Error cleaning up updater " + QString(ex.what()));
	}
	LOG(Info, "Updater files removed");
}

void UpdateInstaller::revert()
{
	LOG(Info, "Reverting installation!");
	std::map<QString, QString>::const_iterator iter = m_backups.begin();
	for (; iter != m_backups.end(); iter++)
	{
		const QString &installedFile = iter->first;
		const QString &backupFile = iter->second;
		LOG(Info, "Restoring " + installedFile);
		if (!m_dryRun)
		{
			if (FileUtils::fileExists(installedFile))
			{
				FileUtils::removeFile(installedFile);
			}
			FileUtils::moveFile(backupFile, installedFile);
		}
	}
}

void UpdateInstaller::installFile(const UpdateScriptFile &file)
{
	const QString absDestPath = FileUtils::makeAbsolute(file.dest, m_installDir);
	const QString absSourcePath = FileUtils::makeAbsolute(file.source, m_packageDir);

	LOG(Info, "Installing file " + absSourcePath + " to " + absDestPath);

	// backup the existing file if any
	backupFile(absDestPath);

	// create the target directory if it does not exist
	const QString destDir = FileUtils::dirname(absDestPath);
	if (!FileUtils::fileExists(destDir))
	{
		LOG(Info, "Destination path missing. Creating " + destDir);
		if (!m_dryRun)
		{
			FileUtils::mkpath(destDir);
		}
	}

	if (!FileUtils::fileExists(absSourcePath))
	{
		throw "Source file does not exist: " + absSourcePath;
	}
	if (!m_dryRun)
	{
		FileUtils::copyFile(absSourcePath, absDestPath);

		if (file.permissions > 0)
		{
			// set the permissions on the newly extracted file
			FileUtils::chmod(absDestPath, file.permissions);
		}
	}
}

void UpdateInstaller::installFiles()
{
	LOG(Info, "Installing files.");
	std::vector<UpdateScriptFile>::const_iterator iter = m_script->filesToInstall().begin();
	int filesInstalled = 0;
	for (; iter != m_script->filesToInstall().end(); iter++)
	{
		installFile(*iter);
		++filesInstalled;
		if (m_observer)
		{
			int toInstallCount = static_cast<int>(m_script->filesToInstall().size());
			double percentage = ((1.0 * filesInstalled) / toInstallCount) * 100.0;
			m_observer->updateProgress(static_cast<int>(percentage));
		}
	}
}

void UpdateInstaller::uninstallFiles()
{
	LOG(Info, "Uninstalling files.");
	std::vector<QString>::const_iterator iter = m_script->filesToUninstall().begin();
	for (; iter != m_script->filesToUninstall().end(); iter++)
	{
		const QString path = FileUtils::makeAbsolute(*iter, m_installDir);
		if (FileUtils::fileExists(path))
		{
			LOG(Info, "Uninstalling " + path);
			if (!m_dryRun)
			{
				FileUtils::removeFile(path);
			}
		}
		else
		{
			LOG(Warn, "Unable to uninstall file " + path + " because it does not exist.");
		}
	}
}

void UpdateInstaller::backupFile(const QString &path)
{
	if (!FileUtils::fileExists(path))
	{
		// no existing file to backup
		return;
	}
	const QString backupPath = path + ".bak";
	LOG(Info, "Backing up file: " + path + " as " + backupPath);
	if (!m_dryRun)
	{
		if (FileUtils::fileExists(backupPath))
		{
			FileUtils::removeFile(backupPath);
		}
		FileUtils::moveFile(path, backupPath);
	}
	m_backups[path] = backupPath;
}

void UpdateInstaller::removeBackups()
{
	LOG(Info, "Removing backups.");
	std::map<QString, QString>::const_iterator iter = m_backups.begin();
	for (; iter != m_backups.end(); iter++)
	{
		const QString &backupFile = iter->second;
		LOG(Info, "Removing " + backupFile);
		if (!m_dryRun)
		{
			FileUtils::removeFile(backupFile);
		}
	}
}

void UpdateInstaller::restartMainApp()
{
	try
	{
		std::list<QString> args;

		if (!m_finishCmd.isEmpty())
		{
			if (!m_finishDir.isEmpty())
			{
				args.push_back("--dir");
				args.push_back(m_finishDir);
			}
			LOG(Info, "Starting main application " + m_finishCmd);
			if (!m_dryRun)
			{
				ProcessUtils::runAsync(m_finishCmd, args);
			}
		}
		else
		{
			LOG(Error, "No main binary specified");
		}
	}
	catch (const std::exception &ex)
	{
		LOG(Error, "Unable to restart main app " + QString(ex.what()));
	}
}

void UpdateInstaller::postInstallUpdate()
{
// perform post-install actions

#ifdef Q_OS_OSX
	// touch the application's bundle directory so that
	// OS X' Launch Services notices any changes in the application's
	// Info.plist file.
	LOG(Info, "Touching " + m_installDir + " to notify OSX of metadata changes.");
	if (!m_dryRun)
	{
		FileUtils::touch(m_installDir);
	}
#endif
}
