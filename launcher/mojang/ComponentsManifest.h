/*
 * Copyright 2023 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once
#include <QString>
#include <map>
#include <set>
#include <QStringList>
#include <QDateTime>
#include <QUrl>
#include "tasks/Task.h"


#include "Path.h"

namespace mojang_files {

struct ComponentVersion {
    int availability_group = 0;
    int availability_progress = 0;

    QString manifest_sha1;
    size_t manifest_size = 0;
    QUrl manifest_url;

    QString version_name;
    QDateTime version_released;
};

struct VersionList {
    std::vector<ComponentVersion> versions;
};

struct ComponentsPlatform {
    std::map<QString, VersionList> components;
};

struct AllPlatformsManifest {
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
