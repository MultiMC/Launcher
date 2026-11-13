#include "Application.h"
#include "BuildConfig.h"

#include "ui/MainWindow.h"
#include "ui/InstanceWindow.h"

#include "ui/instanceview/AccessibleInstanceView.h"

#include "ui/pages/BasePageProvider.h"
#include "ui/pages/global/LauncherPage.h"
#include "ui/pages/global/MinecraftPage.h"
#include "ui/pages/global/JavaPage.h"
#include "ui/pages/global/LanguagePage.h"
#include "ui/pages/global/UpdatesPage.h"
#include "ui/pages/global/AppearancePage.h"
#include "ui/pages/global/ProxyPage.h"
#include "ui/pages/global/ExternalToolsPage.h"
#include "ui/pages/global/PasteEEPage.h"
#include "ui/pages/global/CustomCommandsPage.h"

#include "ui/themes/ThemeManager.h"
#include "ui/themes/TitleBar.h"

#include "ui/setupwizard/SetupWizard.h"
#include "ui/setupwizard/LanguageWizardPage.h"
#include "ui/setupwizard/JavaWizardPage.h"
#include "ui/setupwizard/UpdatesWizardPage.h"
#include "ui/setupwizard/ThemeWizardPage.h"

#include "ui/dialogs/AccountsDialog.h"
#include "ui/dialogs/CustomMessageBox.h"

#include "ui/pagedialog/PageDialog.h"

#include "ApplicationMessage.h"

#include <iostream>

#include <QAccessible>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QTranslator>
#include <QLibraryInfo>
#include <QList>
#include <QStringList>
#include <QDebug>
#include <QStyleFactory>

#include "InstanceList.h"

#include <minecraft/auth/AccountList.h>
#include "icons/IconList.h"
#include "net/HttpMetaCache.h"

#include "skins/CapeCache.h"
#include "skins/SkinsModel.h"

#include "java/JavaUtils.h"

#include "updater/v1/UpdateChecker.h"
#include "updater/v2/Checker.h"
#include "updater/v2/HeadlessUpdate.h"
#include "updater/v2/SystemRequirements.h"

#include "tools/JProfiler.h"
#include "tools/JVisualVM.h"
#include "tools/MCEditTool.h"

#include <xdgicon.h>
#include "settings/INISettingsObject.h"
#include "settings/Setting.h"

#include "translations/TranslationsModel.h"
#include "meta/Index.h"

#include <Commandline.h>
#include <FileSystem.h>
#include <DesktopServices.h>
#include <LocalPeer.h>

#include <sys.h>

#if defined Q_OS_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static const QLatin1String liveCheckFile("live.check");

using namespace Commandline;

#define MACOS_HINT "If you are on macOS Sierra, you might have to move the app to your /Applications or ~/Applications folder. "\
    "This usually fixes the problem and you can move the application elsewhere afterwards.\n"\
    "\n"

namespace {
void appDebugOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    const char *levels = "DWCFIS";
    const QString format("%1 %2 %3\n");

    qint64 msecstotal = APPLICATION->timeSinceStart();
    qint64 seconds = msecstotal / 1000;
    qint64 msecs = msecstotal % 1000;
    QString foo;
    char buf[1025] = {0};
    ::snprintf(buf, 1024, "%5lld.%03lld", seconds, msecs);

    QString out = format.arg(buf).arg(levels[type]).arg(msg);

    APPLICATION->logFile->write(out.toUtf8());
    APPLICATION->logFile->flush();
    QTextStream(stderr) << out.toLocal8Bit();
    fflush(stderr);
}

QString getIdealPlatform(QString currentPlatform) {
    auto info = Sys::getKernelInfo();
    switch(info.kernelType) {
        case Sys::KernelType::Darwin: {
            return "osx64-5.15.2";
        }
        case Sys::KernelType::Windows: {
            // FIXME: 5.15.2 is not stable on Windows, due to a large number of completely unpredictable and hard to reproduce issues
            break;
/*
            if(info.kernelMajor == 6 && info.kernelMinor >= 1) {
                // Windows 7
                return "win32-5.15.2";
            }
            else if (info.kernelMajor > 6) {
                // Above Windows 7
                return "win32-5.15.2";
            }
            else {
                // Below Windows 7
                return "win32";
            }
*/
        }
        case Sys::KernelType::Undetermined:
        case Sys::KernelType::Linux: {
            break;
        }
    }
    return currentPlatform;
}

}

Application::Application(int &argc, char **argv) : QApplication(argc, argv)
{
#if defined Q_OS_WIN32
    // attach the parent console
    if(AttachConsole(ATTACH_PARENT_PROCESS))
    {
        // if attach succeeds, reopen and sync all the i/o
        if(freopen("CON", "w", stdout))
        {
            std::cout.sync_with_stdio();
        }
        if(freopen("CON", "w", stderr))
        {
            std::cerr.sync_with_stdio();
        }
        if(freopen("CON", "r", stdin))
        {
            std::cin.sync_with_stdio();
        }
        auto out = GetStdHandle (STD_OUTPUT_HANDLE);
        DWORD written;
        const char * endline = "\n";
        WriteConsole(out, endline, strlen(endline), &written, NULL);
        consoleAttached = true;
    }
#endif
    setOrganizationName(BuildConfig.LAUNCHER_NAME);
    setOrganizationDomain(BuildConfig.LAUNCHER_DOMAIN);
    setApplicationName(BuildConfig.LAUNCHER_NAME);
    setApplicationDisplayName(BuildConfig.LAUNCHER_DISPLAYNAME);
    setApplicationVersion(BuildConfig.printableVersionString());

    startTime = QDateTime::currentDateTime();

#ifdef Q_OS_LINUX
    {
        QFile osrelease("/proc/sys/kernel/osrelease");
        if (osrelease.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream in(&osrelease);
            auto contents = in.readAll();
            if(
                contents.contains("WSL", Qt::CaseInsensitive) ||
                contents.contains("Microsoft", Qt::CaseInsensitive)
            ) {
                showFatalErrorMessage(
                    "Unsupported system detected!",
                    "Linux-on-Windows distributions are not supported.\n\n"
                    "Please use the Windows binary when playing on Windows."
                );
                return;
            }
        }
    }
#endif

    // Don't quit on hiding the last window
    this->setQuitOnLastWindowClosed(false);

    // Commandline parsing
    QHash<QString, QVariant> args;
    {
        Parser parser(FlagStyle::GNU, ArgumentStyle::SpaceAndEquals);

        // --help
        parser.addSwitch("help");
        parser.addShortOpt("help", 'h');
        parser.addDocumentation("help", "Display this help and exit.");
        // --version
        parser.addSwitch("version");
        parser.addShortOpt("version", 'V');
        parser.addDocumentation("version", "Display program version and exit.");
        // --dir
        parser.addOption("dir");
        parser.addShortOpt("dir", 'd');
        parser.addDocumentation("dir", "Use the supplied folder as application root instead of the binary location (use '.' for current)");
        // --launch
        parser.addOption("launch");
        parser.addShortOpt("launch", 'l');
        parser.addDocumentation("launch", "Launch the specified instance (by instance ID)");
        // --server
        parser.addOption("server");
        parser.addShortOpt("server", 's');
        parser.addDocumentation("server", "Join the specified server on launch (only valid in combination with --launch, mutually exclusive with --world)");
        // --world
        parser.addOption("world");
        parser.addShortOpt("world", 'w');
        parser.addDocumentation("world", "Join the singleplayer world with the specified folder name on launch (only valid in combination with --launch, mutually exclusive with --server, only works with Minecraft 23w14a and later)");
        // --profile
        parser.addOption("profile");
        parser.addShortOpt("profile", 'a');
        parser.addDocumentation("profile", "Use the account specified by its profile name (only valid in combination with --launch)");
        // --offline
        parser.addSwitch("offline");
        parser.addShortOpt("offline", 'o');
        parser.addDocumentation("offline", "Launch offline (only valid in combination with --launch)");
        // --name
        parser.addOption("name");
        parser.addShortOpt("name", 'n');
        parser.addDocumentation("name", "When launching offline, use specified name (only makes sense in combination with --launch and --offline)");
        // --alive
        parser.addSwitch("alive");
        parser.addDocumentation("alive", "Write a small '" + liveCheckFile + "' file after the launcher starts");
        // --update
        parser.addOption("update");
        parser.addDocumentation("update", "Switch to the given tier (latest, testing, stable) and install its build, newer or older, without any window, and exit: "
                                          "0 updated, 1 failed, 2 nothing to do, 3 not for this machine. "
                                          "With --alive, only start and exit. For testing updates");
        // --import
        parser.addOption("import");
        parser.addShortOpt("import", 'I');
        parser.addDocumentation("import", "Import instance from specified zip (local path or URL)");

        // parse the arguments
        try
        {
            args = parser.parse(arguments());
        }
        catch (const ParsingError &e)
        {
            std::cerr << "CommandLineError: " << e.what() << std::endl;
            if(argc > 0)
                std::cerr << "Try '" << argv[0] << " -h' to get help on command line parameters."
                          << std::endl;
            m_status = Application::Failed;
            return;
        }

        // display help and exit
        if (args["help"].toBool())
        {
            std::cout << qPrintable(parser.compileHelp(arguments()[0]));
            m_status = Application::Succeeded;
            return;
        }

        // display version and exit
        if (args["version"].toBool())
        {
            std::cout << "Version " << BuildConfig.printableVersionString().toStdString() << std::endl;
            std::cout << "Git " << BuildConfig.GIT_COMMIT.toStdString() << std::endl;
            m_status = Application::Succeeded;
            return;
        }
    }
    m_instanceIdToLaunch = args["launch"].toString();
    m_serverToJoin = args["server"].toString();
    m_worldToJoin = args["world"].toString();
    m_profileToUse = args["profile"].toString();
    if(args["offline"].toBool()) {
        m_offline = true;
        m_offlineName = args["name"].toString();
    }
    m_liveCheck = args["alive"].toBool();
    m_headlessUpdateTier = args["update"].toString();
    m_zipToImport = args["import"].toUrl();

    QString origcwdPath = QDir::currentPath();
    QString binPath = applicationDirPath();
    QString adjustedBy;
    QString dataPath;
    // change folder
    QString dirParam = args["dir"].toString();
    if (!dirParam.isEmpty())
    {
        // the dir param. it makes multimc data path point to whatever the user specified
        // on command line
        adjustedBy += "Command line " + dirParam;
        dataPath = dirParam;
    }
    else
    {
#if defined(Q_OS_MAC)
        QDir foo(FS::PathCombine(applicationDirPath(), "../../Data"));
        dataPath = foo.absolutePath();
        adjustedBy += "Fallback to special Mac location " + dataPath;
#else
        dataPath = applicationDirPath();
        adjustedBy += "Fallback to binary path " + dataPath;
#endif
    }

    if (!FS::ensureFolderPathExists(dataPath))
    {
        showFatalErrorMessage(
            "The launcher data folder could not be created.",
            QString(
                "The launcher data folder could not be created.\n"
                "\n"
#if defined(Q_OS_MAC)
                MACOS_HINT
#endif
                "Make sure you have the right permissions to the launcher data folder and any folder needed to access it.\n"
                "(%1)\n"
                "\n"
                "The launcher cannot continue until you fix this problem."
            ).arg(dataPath)
        );
        return;
    }
    if (!QDir::setCurrent(dataPath))
    {
        showFatalErrorMessage(
            "The launcher data folder could not be opened.",
            QString(
                "The launcher data folder could not be opened.\n"
                "\n"
#if defined(Q_OS_MAC)
                MACOS_HINT
#endif
                "Make sure you have the right permissions to the launcher data folder.\n"
                "(%1)\n"
                "\n"
                "The launcher cannot continue until you fix this problem."
            ).arg(dataPath)
        );
        return;
    }

    // --world and --server can't be used together
    if(!m_worldToJoin.isEmpty() && !m_serverToJoin.isEmpty())
    {
        std::cerr << "--server and --world are mutually exclusive!" << std::endl;
        m_status = Application::Failed;
        return;
    }

    // all the things invalid when NOT trying to --launch
    if(m_instanceIdToLaunch.isEmpty()) {
        if(!m_serverToJoin.isEmpty())
        {
            std::cerr << "--server can only be used in combination with --launch!" << std::endl;
            m_status = Application::Failed;
            return;
        }

        if(!m_worldToJoin.isEmpty())
        {
            std::cerr << "--world can only be used in combination with --launch!" << std::endl;
            m_status = Application::Failed;
            return;
        }

        if(!m_profileToUse.isEmpty())
        {
            std::cerr << "--account can only be used in combination with --launch!" << std::endl;
            m_status = Application::Failed;
            return;
        }

        if(m_offline)
        {
            std::cerr << "--offline can only be used in combination with --launch!" << std::endl;
            m_status = Application::Failed;
            return;
        }

        if(!m_offlineName.isEmpty())
        {
            std::cerr << "--offlineName can only be used in combination with --launch and --offline!" << std::endl;
            m_status = Application::Failed;
            return;
        }
    }
    else {
        // all the things invalid when trying to --launch
        // online, and offline name is set
        if(!m_offline && !m_offlineName.isEmpty()) {
            std::cerr << "--offlineName can only be used in combination with --launch and --offline!" << std::endl;
            m_status = Application::Failed;
            return;
        }
    }

#if defined(Q_OS_MAC)
    // move user data to new location if on macOS and it still exists in Contents/MacOS
    QDir fi(applicationDirPath());
    QString originalData = fi.absolutePath();
    // if the config file exists in Contents/MacOS, then user data is still there and needs to moved
    if (QFileInfo::exists(FS::PathCombine(originalData, BuildConfig.LAUNCHER_CONFIGFILE)))
    {
        if (!QFileInfo::exists(FS::PathCombine(originalData, "dontmovemacdata")))
        {
            QMessageBox::StandardButton askMoveDialogue;
            askMoveDialogue = QMessageBox::question(
                nullptr,
                BuildConfig.LAUNCHER_DISPLAYNAME,
                "Would you like to move application data to a new data location? It will improve the launcher's performance, but if you switch to older versions it will look like instances have disappeared. If you select no, you can migrate later in settings. You should select yes unless you're commonly switching between different versions (eg. develop and stable).",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes
            );
            if (askMoveDialogue == QMessageBox::Yes)
            {
                qDebug() << "On macOS and found config file in old location, moving user data...";
                QDir dir;
                QStringList dataFiles {
                    "*.log", // Launcher log files: ${Launcher_Name}-@.log
                    "accounts.json",
                    "accounts",
                    "assets",
                    "cache",
                    "icons",
                    "instances",
                    "libraries",
                    "meta",
                    "metacache",
                    "mods",
                    BuildConfig.LAUNCHER_CONFIGFILE,
                    "themes",
                    "translations"
                };
                QDirIterator files(originalData, dataFiles);
                while (files.hasNext()) {
                    QString filePath(files.next());
                    QString fileName(files.fileName());
                    if (!dir.rename(filePath, FS::PathCombine(dataPath, fileName)))
                    {
                        qWarning() << "Failed to move " << fileName;
                    }
                }
            }
            else
            {
                dataPath = originalData;
                QDir::setCurrent(dataPath);
                QFile file(originalData + "/dontmovemacdata");
                file.open(QIODevice::WriteOnly);
            }
        }
        else
        {
            dataPath = originalData;
            QDir::setCurrent(dataPath);
        }
    }
#endif

    /*
     * Establish the mechanism for communication with an already running MultiMC that uses the same data path.
     * If there is one, tell it what the user actually wanted to do and exit.
     * We want to initialize this before logging to avoid messing with the log of a potential already running copy.
     */
    // with the platform: switching variants installs the same build for another one, and the new
    // process must not take the old one, still waiting for it to start, for itself
    auto appID = ApplicationId::fromPathAndVersion(QDir::currentPath(), BuildConfig.printableVersionString() + " on " + BuildConfig.BUILD_PLATFORM);
    {
        // FIXME: you can run the same binaries with multiple data dirs and they won't clash. This could cause issues for updates.
        m_peerInstance = new LocalPeer(this, appID);
        connect(m_peerInstance, &LocalPeer::messageReceived, this, &Application::messageReceived);
        if(m_peerInstance->isClient()) {
            int timeout = 2000;

            if(m_instanceIdToLaunch.isEmpty())
            {
                ApplicationMessage activate;
                activate.command = "activate";
                m_peerInstance->sendMessage(activate.serialize(), timeout);

                if(!m_zipToImport.isEmpty())
                {
                    ApplicationMessage import;
                    import.command = "import";
                    import.args.insert("path", m_zipToImport.toString());
                    m_peerInstance->sendMessage(import.serialize(), timeout);
                }
            }
            else
            {
                ApplicationMessage launch;
                launch.command = "launch";
                launch.args["id"] = m_instanceIdToLaunch;

                if(!m_serverToJoin.isEmpty())
                {
                    launch.args["server"] = m_serverToJoin;
                }
                if(!m_worldToJoin.isEmpty())
                {
                    launch.args["world"] = m_worldToJoin;
                }
                if(!m_profileToUse.isEmpty())
                {
                    launch.args["profile"] = m_profileToUse;
                }
                if(m_offline) {
                    launch.args["offline_enabled"] = "true";
                    launch.args["offline_name"] = m_offlineName;
                }
                m_peerInstance->sendMessage(launch.serialize(), timeout);
            }
            m_status = Application::Succeeded;
            return;
        }
    }

    // init the logger
    {
        static const QString logBase = BuildConfig.LAUNCHER_NAME + "-%0.log";
        auto moveFile = [](const QString &oldName, const QString &newName)
        {
            QFile::remove(newName);
            QFile::copy(oldName, newName);
            QFile::remove(oldName);
        };

        moveFile(logBase.arg(3), logBase.arg(4));
        moveFile(logBase.arg(2), logBase.arg(3));
        moveFile(logBase.arg(1), logBase.arg(2));
        moveFile(logBase.arg(0), logBase.arg(1));

        logFile = std::unique_ptr<QFile>(new QFile(logBase.arg(0)));
        if(!logFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        {
            showFatalErrorMessage(
                "The launcher data folder is not writable!",
                QString(
                    "The launcher couldn't create a log file - the data folder is not writable.\n"
                    "\n"
    #if defined(Q_OS_MAC)
                    MACOS_HINT
    #endif
                    "Make sure you have write permissions to the data folder.\n"
                    "(%1)\n"
                    "\n"
                    "The launcher cannot continue until you fix this problem."
                ).arg(dataPath)
            );
            return;
        }
        qInstallMessageHandler(appDebugOutput);
        qDebug() << "<> Log initialized.";
    }

    // Set up paths
    {
        // Root path is used for updates.
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
        QDir foo(FS::PathCombine(binPath, ".."));
        m_rootPath = foo.absolutePath();
#elif defined(Q_OS_WIN32)
        m_rootPath = binPath;
#elif defined(Q_OS_MAC)
        QDir foo(FS::PathCombine(binPath, "../.."));
        m_rootPath = foo.absolutePath();
        // on macOS, touch the root to force Finder to reload the .app metadata (and fix any icon change issues)
        FS::updateTimestamp(m_rootPath);
#endif

        qDebug() << BuildConfig.LAUNCHER_DISPLAYNAME << ", (c) 2013-2023 " << BuildConfig.LAUNCHER_COPYRIGHT;
        qDebug() << "Version                    : " << BuildConfig.printableVersionString();
        qDebug() << "Git commit                 : " << BuildConfig.GIT_COMMIT;
        qDebug() << "Git refspec                : " << BuildConfig.GIT_REFSPEC;
        {
            auto system = SystemRequirements::detect();
            qDebug() << "Operating system           : " << system.os << system.osVersion;
            qDebug() << "Machine architecture       : " << system.machineArch;
            qDebug() << "Launcher architecture      : " << system.launcherArch;
            if(!system.glibc.isEmpty() || !system.glibcxx.isEmpty())
            {
                qDebug() << "glibc, GLIBCXX             : " << system.glibc << system.glibcxx;
            }
            qDebug() << "Qt version                 : " << system.qt;
        }
        if (adjustedBy.size())
        {
            qDebug() << "Work dir before adjustment : " << origcwdPath;
            qDebug() << "Work dir after adjustment  : " << QDir::currentPath();
            qDebug() << "Adjusted by                : " << adjustedBy;
        }
        else
        {
            qDebug() << "Work dir                   : " << QDir::currentPath();
        }
        qDebug() << "Binary path                : " << binPath;
        qDebug() << "Application root path      : " << m_rootPath;
        if(!m_instanceIdToLaunch.isEmpty())
        {
            qDebug() << "ID of instance to launch   : " << m_instanceIdToLaunch;
        }
        if(!m_serverToJoin.isEmpty())
        {
            qDebug() << "Address of server to join  :" << m_serverToJoin;
        }
        if(!m_worldToJoin.isEmpty())
        {
            qDebug() << "Name of world to join      :" << m_worldToJoin;
        }
        qDebug() << "<> Paths set.";
    }

    do // once
    {
        if(m_liveCheck)
        {
            QFile check(liveCheckFile);
            if(!check.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                qWarning() << "Could not open" << liveCheckFile << "for writing!";
                break;
            }
            auto payload = appID.toString().toUtf8();
            if(check.write(payload) != payload.size())
            {
                qWarning() << "Could not write into" << liveCheckFile << "!";
                check.remove();
                break;
            }
            check.close();
        }
    } while(false);

    // Initialize application settings
    {
        m_settings.reset(new INISettingsObject(BuildConfig.LAUNCHER_CONFIGFILE, this));
        // Updates
        m_settings->registerSetting("AutoUpdate", true);
        // Updater v2's tier: latest, testing or stable. Not the old channel setting, which
        // develop users have and would put them on Latest. Empty until it is chosen (in the
        // setup wizard, UpdatesWizardPage); until then it is Stable.
        m_settings->registerSetting("UpdateTier", QString());
        // Updater v2's variant the user chose to stay on or go back to; empty: none, the
        // Launcher moves along `supersedes` (Design/UpdaterFormats.md)
        m_settings->registerSetting("UpdateVariant", QString());
        // the last build an automatic check said can't be installed here, so it's said once
        m_settings->registerSetting("UpdateNotPossibleBuild", 0);
        // updater v2's checks, to notice that they keep failing (UpdaterV2::CheckHistory):
        // when one last got a verified answer, when they started failing, and how many have
        // failed since (times in seconds since 1970, 0 for none)
        m_settings->registerSetting("UpdateLastSuccess", 0);
        m_settings->registerSetting("UpdateFailingSince", 0);
        m_settings->registerSetting("UpdateFailedChecks", 0);
        // when the deprecation notice for this installation's variant was last shown (seconds)
        m_settings->registerSetting("UpdateDeprecationNotice", 0);

        // Theming
        m_settings->registerSetting("IconTheme", QString("pe_colored"));
        // ThemeManager's; empty until chosen (in the setup wizard, ThemeWizardPage), then the
        // defaults apply. Not the old ApplicationTheme, whose System was the platform's style.
        m_settings->registerSetting("Theme", QString());
        m_settings->registerSetting("ThemeStyle", QString());
        m_settings->registerSetting("AccentSource", QString());
        m_settings->registerSetting("AccentColor", QString());

        // Notifications
        m_settings->registerSetting("ShownNotifications", QString());
        // questions with a "Don't ask again" (brought back by "Reset hidden notifications")
        m_settings->registerSetting("ConfirmLogClear", true);

        // Remembered state
        m_settings->registerSetting("LastUsedGroupForNewInstance", QString());

        QString defaultMonospace;
        int defaultSize = 11;
#ifdef Q_OS_WIN32
        defaultMonospace = "Courier";
        defaultSize = 10;
#elif defined(Q_OS_MAC)
        defaultMonospace = "Menlo";
#else
        defaultMonospace = "Monospace";
#endif

        // resolve the font so the default actually matches
        QFont consoleFont;
        consoleFont.setFamily(defaultMonospace);
        consoleFont.setStyleHint(QFont::Monospace);
        consoleFont.setFixedPitch(true);
        QFontInfo consoleFontInfo(consoleFont);
        QString resolvedDefaultMonospace = consoleFontInfo.family();
        QFont resolvedFont(resolvedDefaultMonospace);
        qDebug() << "Detected default console font:" << resolvedDefaultMonospace
            << ", substitutions:" << resolvedFont.substitutions().join(',');

        m_settings->registerSetting("ConsoleFont", resolvedDefaultMonospace);
        m_settings->registerSetting("ConsoleFontSize", defaultSize);
        m_settings->registerSetting("ConsoleMaxLines", 100000);
        m_settings->registerSetting("ConsoleOverflowStop", true);

        // Folders
        m_settings->registerSetting("InstanceDir", "instances");
        m_settings->registerSetting({"CentralModsDir", "ModsDir"}, "mods");
        m_settings->registerSetting("IconsDir", "icons");
        m_settings->registerSetting("SkinsDir", "skins");

        // Editors
        m_settings->registerSetting("JsonEditor", QString());

        // Language
        m_settings->registerSetting("Language", QString());

        // Console
        m_settings->registerSetting("ShowConsole", false);
        m_settings->registerSetting("AutoCloseConsole", false);
        m_settings->registerSetting("ShowConsoleOnError", true);
        m_settings->registerSetting("LogPrePostOutput", true);

        // Window Size
        m_settings->registerSetting({"LaunchMaximized", "MCWindowMaximize"}, false);
        m_settings->registerSetting({"MinecraftWinWidth", "MCWindowWidth"}, 854);
        m_settings->registerSetting({"MinecraftWinHeight", "MCWindowHeight"}, 480);

        // Proxy Settings
        m_settings->registerSetting("ProxyType", "None");
        m_settings->registerSetting({"ProxyAddr", "ProxyHostName"}, "127.0.0.1");
        m_settings->registerSetting("ProxyPort", 8080);
        m_settings->registerSetting({"ProxyUser", "ProxyUsername"}, "");
        m_settings->registerSetting({"ProxyPass", "ProxyPassword"}, "");

        // Memory
        m_settings->registerSetting({"MinMemAlloc", "MinMemoryAlloc"}, 512);
        m_settings->registerSetting({"MaxMemAlloc", "MaxMemoryAlloc"}, 1024);
        m_settings->registerSetting("PermGen", 128);

        // Java Settings
        m_settings->registerSetting("JavaPath", "");
        m_settings->registerSetting("JavaTimestamp", 0);
        m_settings->registerSetting("JavaArchitecture", "");
        m_settings->registerSetting("JavaVersion", "");
        m_settings->registerSetting("JavaVendor", "");
        m_settings->registerSetting("LastHostname", "");
        m_settings->registerSetting("JvmArgs", "");

        // Native library workarounds
        m_settings->registerSetting("UseNativeOpenAL", false);
        m_settings->registerSetting("UseNativeGLFW", false);

        // Game time
        m_settings->registerSetting("ShowGameTime", true);
        m_settings->registerSetting("ShowGlobalGameTime", true);
        m_settings->registerSetting("RecordGameTime", true);
        m_settings->registerSetting("ShowGameTimeHours", false);

        // Minecraft launch method
        m_settings->registerSetting("MCLaunchMethod", "LauncherPart");

        // Minecraft offline player name
        m_settings->registerSetting("LastOfflinePlayerName", "");

        // Wrapper command for launch
        m_settings->registerSetting("WrapperCommand", "");

        // Custom Commands
        m_settings->registerSetting({"PreLaunchCommand", "PreLaunchCmd"}, "");
        m_settings->registerSetting({"PostExitCommand", "PostExitCmd"}, "");

        // The cat
        m_settings->registerSetting("TheCat", false);

        m_settings->registerSetting("InstSortMode", "Name");
        m_settings->registerSetting("SelectedInstance", QString());

        // Window state and geometry
        m_settings->registerSetting("MainWindowState", "");
        m_settings->registerSetting("MainWindowGeometry", "");

        m_settings->registerSetting("ConsoleWindowState", "");
        m_settings->registerSetting("ConsoleWindowGeometry", "");

        m_settings->registerSetting("SettingsGeometry", "");

        m_settings->registerSetting("PagedGeometry", "");

        m_settings->registerSetting("NewInstanceGeometry", "");

        m_settings->registerSetting("UpdateDialogGeometry", "");

        m_settings->registerSetting("AccountsDialogGeometry", "");
        m_settings->registerSetting("AccountsDialogSplitterState", "");

        // paste.ee API key
        m_settings->registerSetting("PasteEEAPIKey", "multimc");

        // Init page provider
        {
            m_globalSettingsProvider = std::make_shared<GenericPageProvider>(tr("Settings"));
            m_globalSettingsProvider->addPage<LauncherPage>();
            m_globalSettingsProvider->addPage<AppearancePage>();
            if(BuildConfig.UPDATER_ENABLED && BuildConfig.UPDATER_VERSION == 2)
            {
                m_globalSettingsProvider->addPage<UpdatesPage>();
            }
            m_globalSettingsProvider->addPage<MinecraftPage>();
            m_globalSettingsProvider->addPage<JavaPage>();
            m_globalSettingsProvider->addPage<LanguagePage>();
            m_globalSettingsProvider->addPage<CustomCommandsPage>();
            m_globalSettingsProvider->addPage<ProxyPage>();
            m_globalSettingsProvider->addPage<ExternalToolsPage>();
            m_globalSettingsProvider->addPage<PasteEEPage>();
        }
        qDebug() << "<> Settings loaded.";
    }

#ifndef QT_NO_ACCESSIBILITY
    QAccessible::installFactory(groupViewAccessibleFactory);
#endif /* !QT_NO_ACCESSIBILITY */

    // initialize network access and proxy setup
    {
        m_network = new QNetworkAccessManager();
        QString proxyTypeStr = settings()->get("ProxyType").toString();
        QString addr = settings()->get("ProxyAddr").toString();
        int port = settings()->get("ProxyPort").value<qint16>();
        QString user = settings()->get("ProxyUser").toString();
        QString pass = settings()->get("ProxyPass").toString();
        updateProxySettings(proxyTypeStr, addr, port, user, pass);
        qDebug() << "<> Network done.";
    }

    // load translations
    {
        m_translations.reset(new TranslationsModel("translations"));
        auto bcp47Name = m_settings->get("Language").toString();
        m_translations->selectLanguage(bcp47Name);
        qDebug() << "Your language is" << bcp47Name;
        qDebug() << "<> Translations loaded.";
    }

    // initialize the updater
    if(BuildConfig.UPDATER_ENABLED && BuildConfig.UPDATER_VERSION == 2)
    {
        UpdaterV2::Config config;
        config.site = BuildConfig.FILES_MIRRORS.first();
        config.rootKeys = BuildConfig.UPDATER_ROOT_KEYS;
        config.floorKeys = BuildConfig.UPDATER_KEYS_FLOOR;
        config.serialFloor = BuildConfig.UPDATER_SERIAL_FLOOR;
        config.variant = BuildConfig.UPDATER_VARIANT;
        config.currentBuild = BuildConfig.VERSION_BUILD;
        // next to the Launcher's settings
        config.statePath = QDir("updater").absoluteFilePath("state.json");
        config.system = SystemRequirements::detect();
        m_updaterV2.reset(new UpdaterV2::Checker(m_network, config));
        auto setting = m_settings->getSetting("UpdateVariant");
        m_updaterV2->setPreferredVariant(setting->get().toString());
        connect(setting.get(), &Setting::SettingChanged, [&](const Setting &, QVariant value)
        {
            m_updaterV2->setPreferredVariant(value.toString());
        });
        qDebug() << "<> Updater v2 started.";
    }
    else if(BuildConfig.UPDATER_ENABLED)
    {
        auto platform = getIdealPlatform(BuildConfig.BUILD_PLATFORM);
        auto channelUrl = BuildConfig.UPDATER_BASE + platform + "/channels.json";
        qDebug() << "Initializing updater with platform: " << platform << " -- " << channelUrl;
        m_updateChecker.reset(new UpdaterV1::UpdateChecker(m_network, channelUrl, BuildConfig.VERSION_BUILD));
        qDebug() << "<> Updater started.";
    }

    // Instance icons
    {
        auto setting = APPLICATION->settings()->getSetting("IconsDir");
        QStringList instFolders =
        {
            ":/icons/multimc/32x32/instances/",
            ":/icons/multimc/50x50/instances/",
            ":/icons/multimc/128x128/instances/",
            ":/icons/multimc/scalable/instances/"
        };
        m_icons.reset(new IconList(instFolders, setting->get().toString()));
        connect(setting.get(), &Setting::SettingChanged,[&](const Setting &, QVariant value)
        {
            m_icons->directoryChanged(value.toString());
        });
        qDebug() << "<> Instance icons intialized.";
    }

    // Icon themes
    {
        // TODO: icon themes and instance icons do not mesh well together. Rearrange and fix discrepancies!
        // set icon theme search path!
        auto searchPaths = QIcon::themeSearchPaths();
        searchPaths.append("iconthemes");
        QIcon::setThemeSearchPaths(searchPaths);
        // custom icon themes needn't have every icon (the mod platforms' logos, say), and
        // often inherit from themes the launcher doesn't have: the rest come from ours
        XdgIcon::setFallbackThemeName("multimc");
        qDebug() << "<> Icon themes initialized.";
    }

    // Initialize widget themes
    {
        m_themeManager.reset(new ThemeManager(m_settings));
        installEventFilter(new TitleBarWatcher(this));
        qDebug() << "<> Widget themes initialized.";
    }

    // Skins
    {
        auto setting = APPLICATION->settings()->getSetting("SkinsDir");
        m_skinsModel.reset(new SkinsModel(setting->get().toString()));
        connect(setting.get(), &Setting::SettingChanged,[&](const Setting &, QVariant value)
        {
            m_skinsModel->directoryChanged(value.toString());
        });
        qDebug() << "<> Skins intialized.";
    }

    // initialize and load all instances
    {
        auto InstDirSetting = m_settings->getSetting("InstanceDir");
        // instance path: check for problems with '!' in instance path and warn the user in the log
        // and remember that we have to show him a dialog when the gui starts (if it does so)
        QString instDir = InstDirSetting->get().toString();
        qDebug() << "Instance path              : " << instDir;
        if (FS::checkProblemticPathJava(QDir(instDir)))
        {
            qWarning() << "Your instance path contains \'!\' and this is known to cause java problems!";
        }
        m_instances.reset(new InstanceList(m_settings, instDir, this));
        connect(InstDirSetting.get(), &Setting::SettingChanged, m_instances.get(), &InstanceList::on_InstFolderChanged);
        qDebug() << "Loading Instances...";
        m_instances->loadList();
        qDebug() << "<> Instances loaded.";
    }

    // and accounts
    {
        m_accounts.reset(new AccountList(this));
        qDebug() << "Loading accounts...";
        m_accounts->setListFilePath("accounts.json", true);
        m_accounts->loadList();
        m_accounts->fillQueue();
        qDebug() << "<> Accounts loaded.";
    }

    // init the http meta cache
    {
        m_metacache.reset(new HttpMetaCache("metacache"));
        m_metacache->addBase("asset_indexes", QDir("assets/indexes").absolutePath());
        m_metacache->addBase("asset_objects", QDir("assets/objects").absolutePath());
        m_metacache->addBase("versions", QDir("versions").absolutePath());
        m_metacache->addBase("libraries", QDir("libraries").absolutePath());
        m_metacache->addBase("minecraftforge", QDir("mods/minecraftforge").absolutePath());
        m_metacache->addBase("fmllibs", QDir("mods/minecraftforge/libs").absolutePath());
        m_metacache->addBase("liteloader", QDir("mods/liteloader").absolutePath());
        m_metacache->addBase("general", QDir("cache").absolutePath());
        m_metacache->addBase("ATLauncherPacks", QDir("cache/ATLauncherPacks").absolutePath());
        m_metacache->addBase("FTBPacks", QDir("cache/FTBPacks").absolutePath());
        m_metacache->addBase("TechnicPacks", QDir("cache/TechnicPacks").absolutePath());
        m_metacache->addBase("ModrinthPacks", QDir("cache/ModrinthPacks").absolutePath());
        m_metacache->addBase("root", QDir::currentPath());
        m_metacache->addBase("translations", QDir("translations").absolutePath());
        m_metacache->addBase("icons", QDir("cache/icons").absolutePath());
        m_metacache->addBase("meta", QDir("meta").absolutePath());
        m_metacache->Load();
        qDebug() << "<> Cache initialized.";
    }

    // now we have network, download translation updates
    m_translations->downloadIndex();

    //FIXME: what to do with these?
    m_profilers.insert("jprofiler", std::shared_ptr<BaseProfilerFactory>(new JProfilerFactory()));
    m_profilers.insert("jvisualvm", std::shared_ptr<BaseProfilerFactory>(new JVisualVMFactory()));
    for (auto profiler : m_profilers.values())
    {
        profiler->registerSettings(m_settings);
    }

    // Create the MCEdit thing... why is this here?
    {
        m_mcedit.reset(new MCEditTool(m_settings));
    }

    connect(this, &Application::aboutToQuit, [this](){
        if(m_instances)
        {
            // save any remaining instance state
            m_instances->saveNow();
        }
        if(logFile)
        {
            logFile->flush();
            logFile->close();
        }
    });

    {
        setIconTheme(settings()->get("IconTheme").toString());
        qDebug() << "<> Icon theme set.";
        m_themeManager->apply(m_themeManager->savedChoice());
        qDebug() << "<> Application theme set.";
    }

    if(!m_headlessUpdateTier.isEmpty())
    {
        startHeadlessUpdate();
        return;
    }
    if(createSetupWizard())
    {
        return;
    }
    performMainStartupAction();
}

void Application::startHeadlessUpdate()
{
    if(m_liveCheck)
    {
        // Restarted by the installer of a headless update: having started is all it waits
        // for. It also checks that this process still runs (LiveCheck::waitFor, every 250 ms),
        // so stay a while before exiting.
        qDebug() << "Update (headless): started with --alive, exiting in 5 seconds";
        m_status = Application::Initialized;
        QTimer::singleShot(5000, this, []() { QCoreApplication::exit(0); });
        return;
    }
    UpdaterV2::Tier tier;
    if(!UpdaterV2::parseTier(m_headlessUpdateTier, tier))
    {
        std::cerr << "Update (headless): unknown tier " << m_headlessUpdateTier.toStdString() << std::endl;
        m_status = Application::Failed;
        return;
    }
    if(!m_updaterV2)
    {
        // a build that can't update itself with updater 2: exactly what this mode must catch
        qWarning() << "Update (headless): this build doesn't use updater 2 (enabled:" << BuildConfig.UPDATER_ENABLED << "version:"
                   << BuildConfig.UPDATER_VERSION << ")";
        std::cerr << "Update (headless): this build doesn't use updater 2" << std::endl;
        m_status = Application::Failed;
        return;
    }
    m_status = Application::Initialized;
    auto update = new UpdaterV2::HeadlessUpdate(m_updaterV2.get(), m_network, BuildConfig.FILES_MIRRORS.first(), root(), tier, this);
    // once the event loop runs, so that exiting works even when the check ends right away
    QTimer::singleShot(0, update, &UpdaterV2::HeadlessUpdate::start);
}

bool Application::createSetupWizard()
{
    bool javaRequired = [&]()
    {
        QString currentHostName = QHostInfo::localHostName();
        QString oldHostName = settings()->get("LastHostname").toString();
        if (currentHostName != oldHostName)
        {
            settings()->set("LastHostname", currentHostName);
            return true;
        }
        QString currentJavaPath = settings()->get("JavaPath").toString();
        QString actualPath = FS::ResolveExecutable(currentJavaPath);
        if (actualPath.isNull())
        {
            return true;
        }
        return false;
    }();
    bool languageRequired = [&]()
    {
        if (settings()->get("Language").toString().isEmpty())
            return true;
        return false;
    }();
    // updater v2's tier, until it is chosen; existing installs choose once too
    bool updatesRequired = BuildConfig.UPDATER_ENABLED && BuildConfig.UPDATER_VERSION == 2
        && settings()->get("UpdateTier").toString().isEmpty();
    // the theme, until it is chosen; existing installs choose once too
    bool themeRequired = !m_themeManager->themeChosen();
    bool wizardRequired = javaRequired || languageRequired || updatesRequired || themeRequired;

    if(wizardRequired)
    {
        m_setupWizard = new SetupWizard(nullptr);
        if (languageRequired)
        {
            m_setupWizard->addPage(new LanguageWizardPage(m_setupWizard));
        }
        if (themeRequired)
        {
            m_setupWizard->addPage(new ThemeWizardPage(m_setupWizard));
        }
        if (updatesRequired)
        {
            m_setupWizard->addPage(new UpdatesWizardPage(m_setupWizard));
        }
        if (javaRequired)
        {
            m_setupWizard->addPage(new JavaWizardPage(m_setupWizard));
        }
        connect(m_setupWizard, &QDialog::finished, this, &Application::setupWizardFinished);
        m_setupWizard->show();
        return true;
    }
    return false;
}

void Application::setupWizardFinished(int status)
{
    qDebug() << "Wizard result =" << status;
    performMainStartupAction();
}

void Application::performMainStartupAction()
{
    m_status = Application::Initialized;
    if(!m_instanceIdToLaunch.isEmpty())
    {
        auto inst = instances()->getInstanceById(m_instanceIdToLaunch);
        if(inst)
        {
            QuickPlayTargetPtr serverOrWorldToJoin = nullptr;
            MinecraftAccountPtr accountToUse = nullptr;
            bool offline = m_offline;

            qDebug() << "<> Instance" << m_instanceIdToLaunch << "launching";
            if(!m_serverToJoin.isEmpty())
            {
                // FIXME: validate the server string
                serverOrWorldToJoin.reset(new QuickPlayTarget(QuickPlayTarget::parseMultiplayer(m_serverToJoin)));
                qDebug() << "   Launching with server" << m_serverToJoin;
            }

            if(!m_worldToJoin.isEmpty())
            {
                serverOrWorldToJoin.reset(new QuickPlayTarget(QuickPlayTarget::parseSingleplayer(m_worldToJoin)));
                qDebug() << "   Launching with world" << m_worldToJoin;
            }

            if(!m_profileToUse.isEmpty())
            {
                int dummyRow;
                if(!accounts()->getAccountByProfileName(m_profileToUse, accountToUse, dummyRow)) {
                    return;
                }
                qDebug() << "   Launching with account" << m_profileToUse;
            }

            launch(inst, !offline, nullptr, serverOrWorldToJoin, accountToUse, m_offlineName);
            return;
        }
    }
    if(!m_mainWindow)
    {
        // normal main window
        showMainWindow(false);
        qDebug() << "<> Main window shown.";
    }
    if(!m_zipToImport.isEmpty())
    {
        qDebug() << "<> Importing instance from zip:" << m_zipToImport;
        m_mainWindow->droppedURLs({ m_zipToImport });
    }
}

void Application::showFatalErrorMessage(const QString& title, const QString& content)
{
    m_status = Application::Failed;
    auto dialog = CustomMessageBox::selectable(nullptr, title, content, QMessageBox::Critical);
    dialog->exec();
}

Application::~Application()
{
    // Shut down logger by setting the logger function to nothing
    qInstallMessageHandler(nullptr);

#if defined Q_OS_WIN32
    // Detach from Windows console
    if(consoleAttached)
    {
        fclose(stdout);
        fclose(stdin);
        fclose(stderr);
        FreeConsole();
    }
#endif
}

void Application::messageReceived(const QByteArray& message)
{
    if(status() != Initialized)
    {
        qDebug() << "Received message" << message << "while still initializing. It will be ignored.";
        return;
    }

    ApplicationMessage received;
    received.parse(message);

    auto & command = received.command;

    if(command == "activate")
    {
        showMainWindow();
    }
    else if(command == "import")
    {
        QString path = received.args["path"];
        if(path.isEmpty())
        {
            qWarning() << "Received" << command << "message without a zip path/URL.";
            return;
        }
        m_mainWindow->droppedURLs({ QUrl(path) });
    }
    else if(command == "launch")
    {
        QString id = received.args["id"];
        QString server = received.args["server"];
        QString world = received.args["world"];
        QString profile = received.args["profile"];
        bool offline = received.args["offline_enabled"] == "true";
        QString offlineName = received.args["offline_name"];

        InstancePtr instance;
        if(!id.isEmpty()) {
            instance = instances()->getInstanceById(id);
            if(!instance) {
                qWarning() << "Launch command requires an valid instance ID. " << id << "resolves to nothing.";
                return;
            }
        }
        else {
            qWarning() << "Launch command called without an instance ID...";
            return;
        }

        QuickPlayTargetPtr quickPlayTarget = nullptr;
        if(!server.isEmpty()) {
            quickPlayTarget = std::make_shared<QuickPlayTarget>(QuickPlayTarget::parseMultiplayer(server));
        } else if(!world.isEmpty()) {
            quickPlayTarget = std::make_shared<QuickPlayTarget>(QuickPlayTarget::parseSingleplayer(world));
        }

        MinecraftAccountPtr accountObject;
        if(!profile.isEmpty()) {
            int dummyRow;
            if(!accounts()->getAccountByProfileName(profile, accountObject, dummyRow)) {
                qWarning() << "Launch command requires the specified profile to be valid. " << profile << "does not resolve to any account.";
                return;
            }
        }

        launch(
            instance,
            !offline,
            nullptr,
            quickPlayTarget,
            accountObject,
            offlineName
        );
    }
    else
    {
        qWarning() << "Received invalid message" << message;
    }
}

std::shared_ptr<TranslationsModel> Application::translations()
{
    return m_translations;
}

std::shared_ptr<JavaInstallList> Application::javalist()
{
    if (!m_javalist)
    {
        m_javalist.reset(new JavaInstallList());
    }
    return m_javalist;
}

void Application::setIconTheme(const QString& name)
{
    XdgIcon::setThemeName(name);
    // icons load again from the new theme when they are painted next
    for(auto widget: QApplication::allWidgets())
    {
        widget->update();
    }
}

QIcon Application::getThemedIcon(const QString& name)
{
    if(name == "logo") {
        return QIcon(":/logo.svg");
    }
    return XdgIcon::fromTheme(name);
}

bool Application::openJsonEditor(const QString &filename)
{
    const QString file = QDir::current().absoluteFilePath(filename);
    if (m_settings->get("JsonEditor").toString().isEmpty())
    {
        return DesktopServices::openFile(file);
    }
    else
    {
        //return DesktopServices::openFile(m_settings->get("JsonEditor").toString(), file);
        return DesktopServices::run(m_settings->get("JsonEditor").toString(), {file});
    }
}

bool Application::launch(
        InstancePtr instance,
        bool online,
        BaseProfilerFactory *profiler,
        QuickPlayTargetPtr quickPlayTarget,
        MinecraftAccountPtr accountToUse,
        const QString& offlineName
) {
    if(m_updateRunning)
    {
        qDebug() << "Cannot launch instances while an update is running. Please try again when updates are completed.";
    }
    else if(instance->canLaunch())
    {
        auto & extras = m_instanceExtras[instance->id()];
        auto & window = extras.window;
        if(window)
        {
            if(!window->saveAll())
            {
                return false;
            }
        }
        auto & controller = extras.controller;
        controller.reset(new LaunchController());
        controller->setInstance(instance);
        controller->setOnline(online);
        controller->setProfiler(profiler);
        controller->setQuickPlayTarget(quickPlayTarget);
        controller->setAccountToUse(accountToUse);
        controller->setOfflineName(offlineName);
        if(window)
        {
            controller->setParentWidget(window);
        }
        else if(m_mainWindow)
        {
            controller->setParentWidget(m_mainWindow);
        }
        connect(controller.get(), &LaunchController::succeeded, this, &Application::controllerSucceeded);
        connect(controller.get(), &LaunchController::failed, this, &Application::controllerFailed);
        addRunningInstance();
        controller->start();
        return true;
    }
    else if (instance->isRunning())
    {
        showInstanceWindow(instance, "console");
        return true;
    }
    else if (instance->canEdit())
    {
        showInstanceWindow(instance);
        return true;
    }
    return false;
}

bool Application::kill(InstancePtr instance)
{
    if (!instance->isRunning())
    {
        qWarning() << "Attempted to kill instance" << instance->id() << ", which isn't running.";
        return false;
    }
    auto & extras = m_instanceExtras[instance->id()];
    // NOTE: copy of the shared pointer keeps it alive
    auto controller = extras.controller;
    if(controller)
    {
        return controller->abort();
    }
    return true;
}

void Application::addRunningInstance()
{
    m_runningInstances ++;
    if(m_runningInstances == 1)
    {
        emit updateAllowedChanged(false);
    }
}

void Application::subRunningInstance()
{
    if(m_runningInstances == 0)
    {
        qCritical() << "Something went really wrong and we now have less than 0 running instances... WTF";
        return;
    }
    m_runningInstances --;
    if(m_runningInstances == 0)
    {
        emit updateAllowedChanged(true);
    }
}

bool Application::shouldExitNow() const
{
    return m_runningInstances == 0 && m_openWindows == 0;
}

bool Application::updatesAreAllowed()
{
    return m_runningInstances == 0;
}

void Application::updateIsRunning(bool running)
{
    m_updateRunning = running;
}


void Application::controllerSucceeded()
{
    auto controller = qobject_cast<LaunchController *>(QObject::sender());
    if(!controller)
        return;
    auto id = controller->id();
    auto & extras = m_instanceExtras[id];

    // on success, do...
    if (controller->instance()->settings()->get("AutoCloseConsole").toBool())
    {
        if(extras.window)
        {
            extras.window->close();
        }
    }
    extras.controller.reset();
    subRunningInstance();

    // quit when there are no more windows.
    if(shouldExitNow())
    {
        m_status = Status::Succeeded;
        exit(0);
    }
}

void Application::controllerFailed(const QString& error)
{
    Q_UNUSED(error);
    auto controller = qobject_cast<LaunchController *>(QObject::sender());
    if(!controller)
        return;
    auto id = controller->id();
    auto & extras = m_instanceExtras[id];

    // on failure, do... nothing
    extras.controller.reset();
    subRunningInstance();

    // quit when there are no more windows.
    if(shouldExitNow())
    {
        m_status = Status::Failed;
        exit(1);
    }
}

void Application::ShowGlobalSettings(class QWidget* parent, QString open_page)
{
    if(!m_globalSettingsProvider) {
        return;
    }
    emit globalSettingsAboutToOpen();
    {
        SettingsObject::Lock lock(APPLICATION->settings());
        PageDialog dlg(m_globalSettingsProvider.get(), open_page, parent);
        dlg.exec();
    }
    emit globalSettingsClosed();
}

void Application::ShowAccountsDialog(class QWidget* parent)
{
    AccountsDialog dialog(parent);
    dialog.exec();
}


MainWindow* Application::showMainWindow(bool minimized)
{
    if(m_mainWindow)
    {
        m_mainWindow->setWindowState(m_mainWindow->windowState() & ~Qt::WindowMinimized);
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    }
    else
    {
        m_mainWindow = new MainWindow();
        m_mainWindow->restoreState(QByteArray::fromBase64(APPLICATION->settings()->get("MainWindowState").toByteArray()));
        m_mainWindow->restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get("MainWindowGeometry").toByteArray()));
        if(minimized)
        {
            m_mainWindow->showMinimized();
        }
        else
        {
            m_mainWindow->show();
        }

        QTimer::singleShot(0, m_mainWindow, &MainWindow::startupChecks);
        connect(this, &Application::updateAllowedChanged, m_mainWindow, &MainWindow::updatesAllowedChanged);
        connect(m_mainWindow, &MainWindow::isClosing, this, &Application::on_windowClose);
        m_openWindows++;
    }
    return m_mainWindow;
}

InstanceWindow *Application::showInstanceWindow(InstancePtr instance, QString page)
{
    if(!instance)
        return nullptr;
    auto id = instance->id();
    auto & extras = m_instanceExtras[id];
    auto & window = extras.window;

    if(window)
    {
        window->raise();
        window->activateWindow();
    }
    else
    {
        window = new InstanceWindow(instance);
        m_openWindows ++;
        connect(window, &InstanceWindow::isClosing, this, &Application::on_windowClose);
    }
    if(!page.isEmpty())
    {
        window->selectPage(page);
    }
    if(extras.controller)
    {
        extras.controller->setParentWidget(window);
    }
    return window;
}

void Application::on_windowClose()
{
    m_openWindows--;
    auto instWindow = qobject_cast<InstanceWindow *>(QObject::sender());
    if(instWindow)
    {
        auto & extras = m_instanceExtras[instWindow->instanceId()];
        extras.window = nullptr;
        if(extras.controller)
        {
            extras.controller->setParentWidget(m_mainWindow);
        }
    }
    auto mainWindow = qobject_cast<MainWindow *>(QObject::sender());
    if(mainWindow)
    {
        m_mainWindow = nullptr;
    }
    // quit when there are no more windows.
    if(shouldExitNow())
    {
        exit(0);
    }
}

QString Application::msaClientId() const {
    return BuildConfig.MSA_CLIENT_ID;
}

void Application::updateProxySettings(QString proxyTypeStr, QString addr, int port, QString user, QString password)
{
    // Set the application proxy settings.
    if (proxyTypeStr == "SOCKS5")
    {
        QNetworkProxy::setApplicationProxy(
            QNetworkProxy(QNetworkProxy::Socks5Proxy, addr, port, user, password));
    }
    else if (proxyTypeStr == "HTTP")
    {
        QNetworkProxy::setApplicationProxy(
            QNetworkProxy(QNetworkProxy::HttpProxy, addr, port, user, password));
    }
    else if (proxyTypeStr == "None")
    {
        // If we have no proxy set, set no proxy and return.
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    }
    else
    {
        // If we have "Default" selected, set Qt to use the system proxy settings.
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    }

    qDebug() << "Detecting proxy settings...";
    QNetworkProxy proxy = QNetworkProxy::applicationProxy();
    m_network->setProxy(proxy);

    QString proxyDesc;
    if (proxy.type() == QNetworkProxy::NoProxy)
    {
        qDebug() << "Using no proxy is an option!";
        return;
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
    proxyDesc += QString("%1:%2")
                     .arg(proxy.hostName())
                     .arg(proxy.port());
    qDebug() << proxyDesc;
}

shared_qobject_ptr< HttpMetaCache > Application::metacache()
{
    return m_metacache;
}

shared_qobject_ptr<QNetworkAccessManager> Application::network()
{
    return m_network;
}

shared_qobject_ptr<Meta::Index> Application::metadataIndex()
{
    if (!m_metadataIndex)
    {
        m_metadataIndex.reset(new Meta::Index());
    }
    return m_metadataIndex;
}

shared_qobject_ptr<CapeCache> Application::capeCache()
{
    if (!m_capeCache)
    {
        m_capeCache.reset(new CapeCache(this));
    }
    return m_capeCache;
}

QString Application::getJarsPath()
{
    if(m_jarsPath.isEmpty())
    {
        return FS::PathCombine(QCoreApplication::applicationDirPath(), "jars");
    }
    return m_jarsPath;
}
