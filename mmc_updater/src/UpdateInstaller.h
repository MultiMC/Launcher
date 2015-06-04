#pragma once

#include <QProcess>

#include "FileUtils.h"
#include "UpdateScript.h"

#include <map>

class UpdateObserver;

/** Central class responsible for installing updates,
 * launching an elevated copy of the updater if required
 * and restarting the main application once the update
 * is installed.
 */
class UpdateInstaller
{
public:
	void setInstallDir(const QString &path);
	void setPackageDir(const QString &path);
	void setBackupDir(const QString &path);
	void setScript(UpdateScript *script);
	void setWaitPid(Q_PID pid);
	void setDryRun(bool dryRun);
	void setFinishCmd(const QString &cmd);
	void setFinishDir(const QString &dir);

	void setObserver(UpdateObserver *observer);

	void run() throw();

	bool wasFailure() const
	{
		return m_wasFailure;
	}

	void restartMainApp();

private:
	void cleanup();
	void revert();
	void removeBackups();
	bool checkAccess();

	void installFiles();
	void uninstallFiles();
	void installFile(const UpdateScriptFile &file);
	void backupFile(const QString &path);
	void reportError(const QString &error);
	void postInstallUpdate();

	QString friendlyErrorForError(const FileUtils::IOException &ex) const;

	QString m_installDir;
	QString m_packageDir;
	QString m_backupDir;
	QString m_finishCmd;
	QString m_finishDir;
	Q_PID m_waitPid = 0;
	UpdateScript *m_script = nullptr;
	UpdateObserver *m_observer = nullptr;
	std::map<QString, QString> m_backups;
	bool m_dryRun = false;
	bool m_wasFailure = false;
};
