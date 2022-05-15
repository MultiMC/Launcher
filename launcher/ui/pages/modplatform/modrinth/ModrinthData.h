/*
 * Copyright 2022 kb1000
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QString>
#include <QMetaType>
#include <QUrl>
#include <QDateTime>
#include <QVector>

namespace Modrinth {
enum class LoadState {
    NotLoaded = 0,
    Loaded = 1,
    Errored = 2
};

enum class VersionType {
    Alpha,
    Beta,
    Release,
    Unknown
};

struct Download {
    bool valid = false;
    QString filename;
    QString url;
    QString sha1;
    uint64_t size = 0;
    bool primary = false;
};

struct Version {
    QString name;
    Download download;
    QDateTime released;
    VersionType type = VersionType::Unknown;
    bool featured = false;
};

struct Modpack {
    QString id;

    QString name;
    QUrl iconUrl;
    QString author;
    QString description;

    LoadState detailsLoaded = LoadState::NotLoaded;
    QString wikiUrl;
    QString body;

    LoadState versionsLoaded = LoadState::NotLoaded;
    QVector<Version> versions;
};
}

Q_DECLARE_METATYPE(Modrinth::Download)
Q_DECLARE_METATYPE(Modrinth::Version)
Q_DECLARE_METATYPE(Modrinth::Modpack)
