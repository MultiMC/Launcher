
#include "MultiMC.h"
#include <iostream>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QTranslator>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QStringList>

#include "gui/dialogs/VersionSelectDialog.h"
#include "logic/lists/InstanceList.h"
#include "logic/auth/MojangAccountList.h"
#include "logic/lists/IconList.h"
#include "logic/lists/LwjglVersionList.h"
#include "logic/lists/MinecraftVersionList.h"
#include "logic/lists/ForgeVersionList.h"

#include "logic/InstanceLauncher.h"
#include "logic/net/HttpMetaCache.h"

#include "logic/JavaUtils.h"

#include "logic/updater/UpdateChecker.h"

#include "pathutils.h"
#include "cmdutils.h"
#include <inisettingsobject.h>
#include <setting.h>
#include "logger/QsLog.h"
#include <logger/QsLogDest.h>

#include "config.h"
using namespace Util::Commandline;

MultiMC::MultiMC(int &argc, char **argv, const QString &root) : QApplication(argc, argv),
	m_version{VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD, VERSION_CHANNEL, VERSION_BUILD_TYPE}
{
	setOrganizationName("MultiMC");
	setApplicationName("MultiMC5");

	initTranslations();

	// Don't quit on hiding the last window
	this->setQuitOnLastWindowClosed(false);

	// Print app header
	std::cout << "MultiMC 5" << std::endl;
	std::cout << "(c) 2013 MultiMC Contributors" << std::endl << std::endl;

	// Commandline parsing
	QHash<QString, QVariant> args;
	{
		Parser parser(FlagStyle::GNU, ArgumentStyle::SpaceAndEquals);

		// --help
		parser.addSwitch("help");
		parser.addShortOpt("help", 'h');
		parser.addDocumentation("help", "display this help and exit.");
		// --version
		parser.addSwitch("version");
		parser.addShortOpt("version", 'V');
		parser.addDocumentation("version", "display program version and exit.");
		// --dir
		parser.addOption("dir", applicationDirPath());
		parser.addShortOpt("dir", 'd');
		parser.addDocumentation("dir", "use the supplied directory as MultiMC root instead of "
									   "the binary location (use '.' for current)");
		// --update
		parser.addOption("update");
		parser.addShortOpt("update", 'u');
		parser.addDocumentation("update", "replaces the given file with the running executable",
								"<path>");
		// --quietupdate
		parser.addSwitch("quietupdate");
		parser.addShortOpt("quietupdate", 'U');
		parser.addDocumentation("quietupdate",
								"doesn't restart MultiMC after installing updates");
		// WARNING: disabled until further notice
		/*
		// --launch
		parser.addOption("launch");
		parser.addShortOpt("launch", 'l');
		parser.addDocumentation("launch", "tries to launch the given instance", "<inst>");
*/
		// parse the arguments
		try
		{
			args = parser.parse(arguments());
		}
		catch (ParsingError e)
		{
			std::cerr << "CommandLineError: " << e.what() << std::endl;
			std::cerr << "Try '%1 -h' to get help on MultiMC's command line parameters."
					  << std::endl;
			m_status = MultiMC::Failed;
			return;
		}

		// display help and exit
		if (args["help"].toBool())
		{
			std::cout << qPrintable(parser.compileHelp(arguments()[0]));
			m_status = MultiMC::Succeeded;
			return;
		}

		// display version and exit
		if (args["version"].toBool())
		{
			std::cout << "Version " << VERSION_STR << std::endl;
			std::cout << "Git " << GIT_COMMIT << std::endl;
			m_status = MultiMC::Succeeded;
			return;
		}

		// update
		// Note: cwd is always the current executable path!
		if (!args["update"].isNull())
		{
			std::cout << "Performing MultiMC update: " << qPrintable(args["update"].toString())
					  << std::endl;
			QString cwd = QDir::currentPath();
			QDir::setCurrent(applicationDirPath());
			QFile file(applicationFilePath());
			file.copy(args["update"].toString());
			if (args["quietupdate"].toBool())
			{
				m_status = MultiMC::Succeeded;
				return;
			}
			QDir::setCurrent(cwd);
		}
	}

	// change directory
	QDir::setCurrent(args["dir"].toString().isEmpty() ?
				(root.isEmpty() ? QDir::currentPath() : QDir::current().absoluteFilePath(root))
			  : args["dir"].toString());

	// init the logger
	initLogger();

	// load settings
	initGlobalSettings();

	// initialize the updater
	m_updateChecker.reset(new UpdateChecker());

	// and instances
	auto InstDirSetting = m_settings->getSetting("InstanceDir");
	m_instances.reset(new InstanceList(InstDirSetting->get().toString(), this));
	QLOG_INFO() << "Loading Instances...";
	m_instances->loadList();
	connect(InstDirSetting, SIGNAL(settingChanged(const Setting &, QVariant)),
			m_instances.get(), SLOT(on_InstFolderChanged(const Setting &, QVariant)));

	// and accounts
	m_accounts.reset(new MojangAccountList(this));
	QLOG_INFO() << "Loading accounts...";
	m_accounts->setListFilePath("accounts.json", true);
	m_accounts->loadList();

	// init the http meta cache
	initHttpMetaCache();

	// set up a basic autodetected proxy (system default)
	QNetworkProxyFactory::setUseSystemConfiguration(true);

	QLOG_INFO() << "Detecting system proxy settings...";
	auto proxies = QNetworkProxyFactory::systemProxyForQuery();
	if (proxies.size() == 1 && proxies[0].type() == QNetworkProxy::NoProxy)
	{
		QLOG_INFO() << "No proxy found.";
	}
	else for (auto proxy : proxies)
	{
		QString proxyDesc;
		if (proxy.type() == QNetworkProxy::NoProxy)
		{
			QLOG_INFO() << "Using no proxy is an option!";
			continue;
		}
		switch (proxy.type())
		{
		case QNetworkProxy::DefaultProxy:
			proxyDesc = "Default proxy: ";
			break;
		case QNetworkProxy::Socks5Proxy:
			proxyDesc = "Socks5 proxy: ";
			break;
		case QNetworkProxy::HttpProxy:
			proxyDesc = "HTTP proxy: ";
			break;
		case QNetworkProxy::HttpCachingProxy:
			proxyDesc = "HTTP caching: ";
			break;
		case QNetworkProxy::FtpCachingProxy:
			proxyDesc = "FTP caching: ";
			break;
		default:
			proxyDesc = "DERP proxy: ";
			break;
		}
		proxyDesc += QString("%3@%1:%2 pass %4")
						 .arg(proxy.hostName())
						 .arg(proxy.port())
						 .arg(proxy.user())
						 .arg(proxy.password());
		QLOG_INFO() << proxyDesc;
	}

	// create the global network manager
	m_qnam.reset(new QNetworkAccessManager(this));

	// launch instance, if that's what should be done
	// WARNING: disabled until further notice
	/*
	if (!args["launch"].isNull())
	{
		if (InstanceLauncher(args["launch"].toString()).launch())
			m_status = MultiMC::Succeeded;
		else
			m_status = MultiMC::Failed;
		return;
	}
*/
	m_status = MultiMC::Initialized;
}

MultiMC::~MultiMC()
{
	if (m_mmc_translator)
	{
		removeTranslator(m_mmc_translator.get());
	}
	if (m_qt_translator)
	{
		removeTranslator(m_qt_translator.get());
	}
}

void MultiMC::initTranslations()
{
	m_qt_translator.reset(new QTranslator());
	if (m_qt_translator->load("qt_" + QLocale::system().name(),
							  QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
	{
		std::cout << "Loading Qt Language File for "
				  << QLocale::system().name().toLocal8Bit().constData() << "...";
		if (!installTranslator(m_qt_translator.get()))
		{
			std::cout << " failed.";
			m_qt_translator.reset();
		}
		std::cout << std::endl;
	}
	else
	{
		m_qt_translator.reset();
	}

	m_mmc_translator.reset(new QTranslator());
	if (m_mmc_translator->load("mmc_" + QLocale::system().name(),
							   QDir("translations").absolutePath()))
	{
		std::cout << "Loading MMC Language File for "
				  << QLocale::system().name().toLocal8Bit().constData() << "...";
		if (!installTranslator(m_mmc_translator.get()))
		{
			std::cout << " failed.";
			m_mmc_translator.reset();
		}
		std::cout << std::endl;
	}
	else
	{
		m_mmc_translator.reset();
	}
}

void moveFile(const QString &oldName, const QString &newName)
{
	QFile::remove(newName);
	QFile::copy(oldName, newName);
	QFile::remove(oldName);
}
void MultiMC::initLogger()
{
	static const QString logBase = "MultiMC-%0.log";

	moveFile(logBase.arg(3), logBase.arg(4));
	moveFile(logBase.arg(2), logBase.arg(3));
	moveFile(logBase.arg(1), logBase.arg(2));
	moveFile(logBase.arg(0), logBase.arg(1));

	// init the logging mechanism
	QsLogging::Logger &logger = QsLogging::Logger::instance();
	logger.setLoggingLevel(QsLogging::TraceLevel);
	m_fileDestination = QsLogging::DestinationFactory::MakeFileDestination(logBase.arg(0));
	m_debugDestination = QsLogging::DestinationFactory::MakeQDebugDestination();
	logger.addDestination(m_fileDestination.get());
	logger.addDestination(m_debugDestination.get());
	// log all the things
	logger.setLoggingLevel(QsLogging::TraceLevel);
}

void MultiMC::initGlobalSettings()
{
	m_settings.reset(new INISettingsObject("multimc.cfg", this));
	// Updates
	m_settings->registerSetting(new Setting("UseDevBuilds", false));
	m_settings->registerSetting(new Setting("AutoUpdate", true));

	// Folders
	m_settings->registerSetting(new Setting("InstanceDir", "instances"));
	m_settings->registerSetting(new Setting("CentralModsDir", "mods"));
	m_settings->registerSetting(new Setting("LWJGLDir", "lwjgl"));

	// Console
	m_settings->registerSetting(new Setting("ShowConsole", true));
	m_settings->registerSetting(new Setting("AutoCloseConsole", true));

	// Console Colors
	//	m_settings->registerSetting(new Setting("SysMessageColor", QColor(Qt::blue)));
	//	m_settings->registerSetting(new Setting("StdOutColor", QColor(Qt::black)));
	//	m_settings->registerSetting(new Setting("StdErrColor", QColor(Qt::red)));

	// Window Size
	m_settings->registerSetting(new Setting("LaunchMaximized", false));
	m_settings->registerSetting(new Setting("MinecraftWinWidth", 854));
	m_settings->registerSetting(new Setting("MinecraftWinHeight", 480));

	// Memory
	m_settings->registerSetting(new Setting("MinMemAlloc", 512));
	m_settings->registerSetting(new Setting("MaxMemAlloc", 1024));
	m_settings->registerSetting(new Setting("PermGen", 64));

	// Java Settings
	m_settings->registerSetting(new Setting("JavaPath", ""));
	m_settings->registerSetting(new Setting("LastHostname", ""));
	m_settings->registerSetting(new Setting("JvmArgs", ""));

	// Custom Commands
	m_settings->registerSetting(new Setting("PreLaunchCommand", ""));
	m_settings->registerSetting(new Setting("PostExitCommand", ""));

	// The cat
	m_settings->registerSetting(new Setting("TheCat", false));


	m_settings->registerSetting(new Setting("InstSortMode", "Name"));
	m_settings->registerSetting(new Setting("SelectedInstance", QString()));

	// Persistent value for the client ID
	m_settings->registerSetting(new Setting("YggdrasilClientToken", ""));
	QString currentYggID = m_settings->get("YggdrasilClientToken").toString();
	if (currentYggID.isEmpty())
	{
		QUuid uuid = QUuid::createUuid();
		m_settings->set("YggdrasilClientToken", uuid.toString());
	}

	// Window state and geometry
	m_settings->registerSetting(new Setting("MainWindowState", ""));
	m_settings->registerSetting(new Setting("MainWindowGeometry", ""));

	m_settings->registerSetting(new Setting("ConsoleWindowState", ""));
	m_settings->registerSetting(new Setting("ConsoleWindowGeometry", ""));
}

void MultiMC::initHttpMetaCache()
{
	m_metacache.reset(new HttpMetaCache("metacache"));
	m_metacache->addBase("asset_indexes", QDir("assets/indexes").absolutePath());
	m_metacache->addBase("asset_objects", QDir("assets/objects").absolutePath());
	m_metacache->addBase("versions", QDir("versions").absolutePath());
	m_metacache->addBase("libraries", QDir("libraries").absolutePath());
	m_metacache->addBase("minecraftforge", QDir("mods/minecraftforge").absolutePath());
	m_metacache->addBase("skins", QDir("accounts/skins").absolutePath());
	m_metacache->Load();
}

std::shared_ptr<IconList> MultiMC::icons()
{
	if (!m_icons)
	{
		m_icons.reset(new IconList);
	}
	return m_icons;
}

std::shared_ptr<LWJGLVersionList> MultiMC::lwjgllist()
{
	if (!m_lwjgllist)
	{
		m_lwjgllist.reset(new LWJGLVersionList());
	}
	return m_lwjgllist;
}

std::shared_ptr<ForgeVersionList> MultiMC::forgelist()
{
	if (!m_forgelist)
	{
		m_forgelist.reset(new ForgeVersionList());
	}
	return m_forgelist;
}

std::shared_ptr<MinecraftVersionList> MultiMC::minecraftlist()
{
	if (!m_minecraftlist)
	{
		m_minecraftlist.reset(new MinecraftVersionList());
	}
	return m_minecraftlist;
}

std::shared_ptr<JavaVersionList> MultiMC::javalist()
{
	if (!m_javalist)
	{
		m_javalist.reset(new JavaVersionList());
	}
	return m_javalist;
}

#ifdef WINDOWS
#define UPDATER_BIN "updater.exe"
#elif LINUX
#define UPDATER_BIN "updater"
#elif OSX
#define UPDATER_BIN "updater"
#else
#error Unsupported operating system.
#endif

void MultiMC::installUpdates(const QString& updateFilesDir, bool restartOnFinish)
{
	QLOG_INFO() << "Installing updates.";
#if LINUX
	// On Linux, the MultiMC executable file is actually in the bin folder inside the installation directory.
	// This means that MultiMC's *actual* install path is the parent folder.
	// We need to tell the updater to run with this directory as the install path, rather than the bin folder where the executable is.
	// On other operating systems, we'll just use the path to the executable.
	QString appDir = QFileInfo(MMC->applicationDirPath()).dir().path();

	// On Linux, we also need to set the finish command to the launch script, rather than the binary.
	QString finishCmd = PathCombine(appDir, "MultiMC");
#else
	QString appDir = MMC->applicationDirPath();
	QString finishCmd = MMC->applicationFilePath();
#endif

	// Build the command we'll use to run the updater.
	// Note, the above comment about the app dir path on Linux is irrelevant here because the updater binary is always in the
	// same folder as the main binary.
	QString updaterBinary = PathCombine(MMC->applicationDirPath(), UPDATER_BIN);
	QStringList args;
	// ./updater --install-dir $INSTALL_DIR --package-dir $UPDATEFILES_DIR --script $UPDATEFILES_DIR/file_list.xml --wait $PID --mode main
	args << "--install-dir" << appDir;
	args << "--package-dir" << updateFilesDir;
	args << "--script"      << PathCombine(updateFilesDir, "file_list.xml");
	args << "--wait"        << QString::number(MMC->applicationPid());

	if (restartOnFinish)
		args << "--finish-cmd"  << finishCmd;

	QLOG_INFO() << "Running updater with command" << updaterBinary << args.join(" ");

	QProcess::startDetached(updaterBinary, args);

	// Now that we've started the updater, quit MultiMC.
	MMC->quit();
}

void MultiMC::setUpdateOnExit(const QString& updateFilesDir)
{
	m_updateOnExitPath = updateFilesDir;
}

QString MultiMC::getExitUpdatePath() const
{
	return m_updateOnExitPath;
}


#include "MultiMC.moc"
