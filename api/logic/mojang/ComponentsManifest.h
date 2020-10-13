#pragma once
#include <QString>
#include <map>
#include <set>
#include <QStringList>
#include <QDateTime>
#include <QUrl>
#include "tasks/Task.h"


#include "Path.h"

#include "multimc_logic_export.h"

namespace mojang_files {

struct MULTIMC_LOGIC_EXPORT ComponentVersion {
    int availability_group = 0;
    int availability_progress = 0;

    QString manifest_sha1;
    size_t manifest_size = 0;
    QUrl manifest_url;

    QString version_name;
    QDateTime version_released;
};

struct MULTIMC_LOGIC_EXPORT VersionList {
    std::vector<ComponentVersion> versions;
};

struct MULTIMC_LOGIC_EXPORT ComponentsPlatform {
    std::map<QString, VersionList> components;
};

struct MULTIMC_LOGIC_EXPORT AllPlatformsManifest {
    static AllPlatformsManifest fromManifestFile(const QString &path);
    static AllPlatformsManifest fromManifestContents(const QByteArray& contents);

    explicit operator bool() const
    {
        return valid;
    }

    std::map<QString, ComponentsPlatform> platforms;
    bool valid = false;
};

}
