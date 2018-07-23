#pragma once
#include <QString>

/**
 * \brief The Config class holds all the build-time information passed from the build system.
 */
class Config
{
public:
    Config();
    /// The major version number.
    int VERSION_MAJOR;
    /// The minor version number.
    int VERSION_MINOR;
    /// The hotfix number.
    int VERSION_HOTFIX;
    /// The build number.
    int VERSION_BUILD;

    /**
     * The version channel
     * This is used by the updater to determine what channel the current version came from.
     */
    QString VERSION_CHANNEL;

    bool UPDATER_ENABLED = false;

    /// A short string identifying this build's platform. For example, "lin64" or "win32".
    QString BUILD_PLATFORM;

    /// URL for the updater's channel
    QString CHANLIST_URL;

    /// Google analytics ID
    QString ANALYTICS_ID;

    /// URL for notifications
    QString NOTIFICATION_URL;

    /// Used for matching notifications
    QString FULL_VERSION_STR;

    /// The git commit hash of this build
    QString GIT_COMMIT;

    /// The git refspec of this build
    QString GIT_REFSPEC;

    /// This is printed on start to standard output
    QString VERSION_STR;

    /**
     * This is used to fetch the news RSS feed.
     * It defaults in CMakeLists.txt to "http://multimc.org/rss.xml"
     */
    QString NEWS_RSS_URL;

    /**
     * API key you can get from paste.ee when you register an account
     */
    QString PASTE_EE_KEY;

    /**
     * \brief Converts the Version to a string.
     * \return The version number in string format (major.minor.revision.build).
     */
    QString printableVersionString() const;
};

extern Config BuildConfig;
