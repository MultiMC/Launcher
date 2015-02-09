#include "AppInfo.h"
#include "FileUtils.h"
#include "Log.h"
#include "Platform.h"
#include "ProcessUtils.h"
#include "UpdateScript.h"
#include "UpdaterOptions.h"
#include "UpdateInstaller.h"

#include <thread>

#if defined(Q_OS_LINUX)
  #include "UpdateDialogGtkFactory.h"
  #include "UpdateDialogAscii.h"
#endif

#if defined(Q_OS_OSX)
  #include "MacBundle.h"
  #include "UpdateDialogCocoa.h"
#endif

#if defined(Q_OS_WIN)
  #include "UpdateDialogWin32.h"
#endif

#include <iostream>
#include <memory>

#define UPDATER_VERSION "0.16"

UpdateDialog* createUpdateDialog();

void runUpdaterThread(void* arg)
{
#ifdef Q_OS_OSX
	// create an autorelease pool to free any temporary objects
	// created by Cocoa whilst handling notifications from the UpdateInstaller
	void* pool = UpdateDialogCocoa::createAutoreleasePool();
#endif

	try
	{
		UpdateInstaller* installer = static_cast<UpdateInstaller*>(arg);
		installer->run();
	}
	catch (const std::exception& ex)
	{
		LOG(Error,"Unexpected exception " + std::string(ex.what()));
	}

#ifdef Q_OS_OSX
	UpdateDialogCocoa::releaseAutoreleasePool(pool);
#endif
}

#ifdef Q_OS_OSX
extern unsigned char Info_plist[];
extern unsigned int Info_plist_len;

extern unsigned char mac_icns[];
extern unsigned int mac_icns_len;

bool unpackBundle(int argc, char** argv)
{
	MacBundle bundle(FileUtils::tempPath(), QString::fromStdString(AppInfo::name()));
	QString currentExePath = ProcessUtils::currentProcessPath();

	if (currentExePath.contains(bundle.bundlePath()))
	{
		// already running from a bundle
		return false;
	}
	LOG(Info,"Creating bundle " + bundle.bundlePath());

	// create a Mac app bundle
	std::string plistContent(reinterpret_cast<const char*>(Info_plist),Info_plist_len);
	std::string iconContent(reinterpret_cast<const char*>(mac_icns),mac_icns_len);
	bundle.create(plistContent,iconContent,ProcessUtils::currentProcessPath());

	std::list<QString> args;
	for (int i = 1; i < argc; i++)
	{
		args.push_back(argv[i]);
	}
	ProcessUtils::runSync(bundle.executablePath(), args);
	return true;
}
#endif

void setupConsole()
{
#ifdef Q_OS_WIN
	// see http://stackoverflow.com/questions/587767/how-to-output-to-console-in-c-windows
	// and http://www.libsdl.org/cgi/docwiki.cgi/FAQ_Console
	AttachConsole(ATTACH_PARENT_PROCESS);
	freopen( "CON", "w", stdout );
	freopen( "CON", "w", stderr );
#endif
}

int main(int argc, char** argv)
{
#ifdef Q_OS_OSX
	void* pool = UpdateDialogCocoa::createAutoreleasePool();
#endif
	
	Log::instance()->open(AppInfo::logFilePath());

#ifdef Q_OS_OSX
	// when the updater is run for the first time, create a Mac app bundle
	// and re-launch the application from the bundle.  This permits
	// setting up bundle properties (such as application icon)
	if (unpackBundle(argc,argv))
	{
		return 0;
	}
#endif

	UpdaterOptions options;
	options.parse(argc,argv);

	UpdateInstaller installer;
	UpdateScript script;

	if (!options.scriptPath.isEmpty())
	{
		try
		{
			script.parse(options.scriptPath);
			LOG(Info,"Loaded script from " + options.scriptPath);
		}
		catch (std::exception &e)
		{
			LOG(Error, "Unable to load script: " + std::string(e.what()));
			return -1;
		}
	}

	LOG(Info,"started updater. install-dir: " + options.installDir
		+ ", package-dir: " + options.packageDir
		+ ", wait-pid: " + QString::number(options.waitPid)
		+ ", script-path: " + options.scriptPath
		+ ", finish-cmd: " + options.finishCmd
		+ ", finish-dir: " + options.finishDir);

	installer.setInstallDir(options.installDir);
	installer.setPackageDir(options.packageDir);
	installer.setScript(&script);
	installer.setWaitPid(static_cast<Q_PID>(options.waitPid));
	installer.setFinishCmd(options.finishCmd);
	installer.setFinishDir(options.finishDir);
	installer.setDryRun(options.dryRun);

	LOG(Info, "Showing updater UI");
	std::unique_ptr<UpdateDialog> dialog(createUpdateDialog());
	dialog->setAutoClose(true);
	dialog->init(argc, argv);
	installer.setObserver(dialog.get());
	std::thread updaterThread(runUpdaterThread, &installer);
	dialog->exec();
	updaterThread.join();

#ifdef Q_OS_OSX
	UpdateDialogCocoa::releaseAutoreleasePool(pool);
#endif

	return installer.wasFailure() ? 1 : 0;
}

UpdateDialog* createUpdateDialog()
{
#if defined(Q_OS_WIN)
	return new UpdateDialogWin32();
#elif defined(Q_OS_OSX)
	return new UpdateDialogCocoa();
#elif defined(Q_OS_LINUX)
	UpdateDialog* dialog = UpdateDialogGtkFactory::createDialog();
	if (!dialog)
	{
		dialog = new UpdateDialogAscii();
	}
	return dialog;
#endif
}

#ifdef Q_OS_WIN
// application entry point under Windows
int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nCmdShow)
{
	int argc = 0;
	char** argv;
	ProcessUtils::convertWindowsCommandLine(GetCommandLineW(),argc,argv);
	return main(argc,argv);
}
#endif
