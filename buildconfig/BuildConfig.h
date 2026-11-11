#pragma once
#include <QByteArray>
#include <QString>
#include <QStringList>

/**
 * \brief The Config class holds all the build-time information passed from the build system.
 */
class Config
{
public:
    Config();
    QString LAUNCHER_NAME;
    QString LAUNCHER_DISPLAYNAME;
    QString LAUNCHER_COPYRIGHT;
    QString LAUNCHER_DOMAIN;
    QString LAUNCHER_CONFIGFILE;
    QString LAUNCHER_GIT;

    /// The major version number.
    int VERSION_MAJOR;
    /// The minor version number.
    int VERSION_MINOR;
    /// The build number, -1 for a build without one. The version is major.minor.build.
    int VERSION_BUILD;

    /**
     * The version channel
     * This is used by the updater to determine what channel the current version came from.
     */
    QString VERSION_CHANNEL;

    bool UPDATER_ENABLED = false;

    /// A short string identifying this build's platform. For example, "lin64" or "win32".
    QString BUILD_PLATFORM;

    /**
     * Our sites: the base URLs of each, the main server first, then its copies.
     * Downloads from a site fall back to its other base URLs (Net::Mirrors).
     */
    QStringList FILES_MIRRORS;
    QStringList META_MIRRORS;

    /// URL for the updater's channel
    QString UPDATER_BASE;

    /// The updater this build uses: 1, the current one, or 2 (Design/Updater.md in the MultiMC repository)
    int UPDATER_VERSION = 1;
    /// Updater v2: pinned root public keys (hex)
    QStringList UPDATER_ROOT_KEYS;
    /// Updater v2: the keys document trusted without a signature, as its exact bytes
    QByteArray UPDATER_KEYS_FLOOR;
    /// Updater v2: the lowest tags serial accepted
    qint64 UPDATER_SERIAL_FLOOR = 0;
    /// Updater v2: this build's variant ID
    QString UPDATER_VARIANT;


    /// User-Agent to use.
    QString USER_AGENT;

    /// User-Agent to use for uncached requests.
    QString USER_AGENT_UNCACHED;


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
     * This is used to open the news page with the "More news" button.
     * It defaults in CMakeLists.txt to "https://multimc.org/posts.html"
     */
    QString NEWS_URL;

    /**
     * This is used to fetch the news RSS feed.
     * It defaults in CMakeLists.txt to "https://multimc.org/rss.xml"
     */
    QString NEWS_RSS_URL;

    /**
     * API key you can get from paste.ee when you register an account
     */
    QString PASTE_EE_KEY;

    /**
     * API key from registering an application in Microsoft Azure
     * See: https://portal.azure.com/#view/Microsoft_AAD_RegisteredApps/ApplicationsListBlade
     */
    QString MSA_CLIENT_ID;

    /**
     * Client ID you can get from Imgur when you register an application
     */
    QString IMGUR_CLIENT_ID;

    /**
     * Metadata repository URL prefix
     */
    QString META_URL;

    QString BUG_TRACKER_URL;
    QString DISCORD_URL;
    QString SUBREDDIT_URL;
    QString PATREON_URL;

    QString RESOURCE_BASE = "https://resources.download.minecraft.net/";
    QString LIBRARY_BASE = "https://libraries.minecraft.net/";
    QString API_BASE = "https://api.minecraftservices.com";
    QString SESSION_BASE = "https://sessionserver.mojang.com";
    QString TEXTURE_BASE = "http://textures.minecraft.net";
    QString IMGUR_BASE_URL = "https://api.imgur.com/3/";
    /// Where the Launcher can be downloaded: for an installation that can no longer update itself
    QString WEBSITE_URL = "https://multimc.org/";
    QString FMLLIBS_BASE_URL;
    QString PATRONS_URL;

    QString TRANSLATIONS_BASE_URL;

    QString LEGACY_FTB_CDN_BASE_URL = "https://dist.creeper.host/FTB2/";

    QString ATL_DOWNLOAD_SERVER_URL = "https://download.nodecdn.net/containers/atl/";

    QString TECHNIC_API_BASE_URL = "https://api.technicpack.net/";
    /**
     * The build that is reported to the Technic API.
     */
    QString TECHNIC_API_BUILD = "multimc";

    /**
     * \brief Converts the Version to a string.
     * \return The version number in string format (major.minor.revision.build).
     */
    QString printableVersionString() const;
};

extern const Config BuildConfig;
