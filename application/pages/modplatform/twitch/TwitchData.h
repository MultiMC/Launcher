#pragma once

#include <QString>
#include <QList>

namespace Twitch {

struct Author {
    QString name;
    QString url;
};

struct File {
    int addonId;
    int fileId;
    QString version;
    QString mcVersion;
    QString downloadUrl;
};

struct Addon
{
    bool broken = true;
    int addonId = 0;

    QString name;
    QString description;
    QList<Author> authors;
    QString mcVersion;
    QString logoName;
    QString logoUrl;
    QString websiteUrl;

    File latestFile;

    QList<File> files;
};

enum SearchType
{
    Modpack = 4471,
    Mod = 6,
};
}

Q_DECLARE_METATYPE(Twitch::Addon)
