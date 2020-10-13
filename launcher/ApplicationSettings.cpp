
#include "ApplicationSettings.h"
#include <BuildConfig.h>
#include <QFontInfo>

ApplicationSettings::ApplicationSettings(const QString & path, QObject * parent) : INISettingsObject(path, parent)
{
    // Updates
    registerSetting("AutoUpdate", true);

    // Theming
    registerSetting("IconTheme", QString("multimc"));
    registerSetting("ApplicationTheme", QString("system"));

    // Notifications
    registerSetting("ShownNotifications", QString());

    // Remembered state
    registerSetting("LastUsedGroupForNewInstance", QString());

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
    qDebug() << "Detected default console font:" << resolvedDefaultMonospace << ", substitutions:" << resolvedFont.substitutions().join(',');

    registerSetting("ConsoleFont", resolvedDefaultMonospace);
    registerSetting("ConsoleFontSize", defaultSize);
    registerSetting("ConsoleMaxLines", 100000);
    registerSetting("ConsoleOverflowStop", true);

    // Folders
    registerSetting("InstanceDir", "instances");
    registerSetting({"CentralModsDir", "ModsDir"}, "mods");
    registerSetting("IconsDir", "icons");

    // Editors
    registerSetting("JsonEditor", QString());

    // Language
    registerSetting("Language", QString());

    // Console
    registerSetting("ShowConsole", false);
    registerSetting("AutoCloseConsole", false);
    registerSetting("ShowConsoleOnError", true);
    registerSetting("LogPrePostOutput", true);

    // Window Size
    registerSetting({"LaunchMaximized", "MCWindowMaximize"}, false);
    registerSetting({"MinecraftWinWidth", "MCWindowWidth"}, 854);
    registerSetting({"MinecraftWinHeight", "MCWindowHeight"}, 480);

    // Proxy Settings
    registerSetting("ProxyType", "None");
    registerSetting({"ProxyAddr", "ProxyHostName"}, "127.0.0.1");
    registerSetting("ProxyPort", 8080);
    registerSetting({"ProxyUser", "ProxyUsername"}, "");
    registerSetting({"ProxyPass", "ProxyPassword"}, "");

    // Memory
    registerSetting({"MinMemAlloc", "MinMemoryAlloc"}, 512);
    registerSetting({"MaxMemAlloc", "MaxMemoryAlloc"}, 1024);
    registerSetting("PermGen", 128);

    // Java Settings
    registerSetting("JavaPath", "");
    registerSetting("JavaTimestamp", 0);
    registerSetting("JavaArchitecture", "");
    registerSetting("JavaVersion", "");
    registerSetting("JavaVendor", "");
    registerSetting("LastHostname", "");
    registerSetting("JvmArgs", "");

    // Native library workarounds
    registerSetting("UseNativeOpenAL", false);
    registerSetting("UseNativeGLFW", false);

    // Game time
    registerSetting("ShowGameTime", true);
    registerSetting("ShowGlobalGameTime", true);
    registerSetting("RecordGameTime", true);
    registerSetting("ShowGameTimeHours", false);

    // Minecraft launch method
    registerSetting("MCLaunchMethod", "LauncherPart");

    // Minecraft offline player name
    registerSetting("LastOfflinePlayerName", "");

    // Wrapper command for launch
    registerSetting("WrapperCommand", "");

    // Custom Commands
    registerSetting({"PreLaunchCommand", "PreLaunchCmd"}, "");
    registerSetting({"PostExitCommand", "PostExitCmd"}, "");

    // The cat
    registerSetting("TheCat", false);

    registerSetting("InstSortMode", "Name");
    registerSetting("SelectedInstance", QString());

    // Window state and geometry
    registerSetting("MainWindowState", "");
    registerSetting("MainWindowGeometry", "");

    registerSetting("ConsoleWindowState", "");
    registerSetting("ConsoleWindowGeometry", "");

    registerSetting("SettingsGeometry", "");

    registerSetting("PagedGeometry", "");

    registerSetting("NewInstanceGeometry", "");

    registerSetting("UpdateDialogGeometry", "");

    registerSetting("JREClientDialogGeometry", "");

    // paste.ee API key
    registerSetting("PasteEEAPIKey", "multimc");

    if(!BuildConfig.ANALYTICS_ID.isEmpty())
    {
        // Analytics
        registerSetting("Analytics", true);
        registerSetting("AnalyticsSeen", 0);
        registerSetting("AnalyticsClientID", QString());
    }
}
