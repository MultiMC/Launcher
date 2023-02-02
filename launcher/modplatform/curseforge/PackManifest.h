#pragma once

#include <QString>
#include <QVector>
#include <QUrl>
#include <QJsonObject>

namespace CurseForge
{
struct File
{
    bool parse(QJsonObject fileObject);

    int projectId = 0;
    int fileId = 0;

    bool required = true;

    QString sha1;
    QString md5;

    // our
    bool resolved = false;
    QString fileName;
    QUrl url;
    QString targetFolder = QLatin1Literal("mods");
};

struct Modloader
{
    QString id;
    bool primary = false;
};

struct Minecraft
{
    QString version;
    QString libraries;
    QVector<CurseForge::Modloader> modLoaders;
};

struct Manifest
{
    QString manifestType;
    int manifestVersion = 0;
    CurseForge::Minecraft minecraft;
    QString name;
    QString version;
    QString author;
    QMap<int, CurseForge::File> files;
    QString overrides;
};

void loadManifest(CurseForge::Manifest & m, const QString &filepath);
}
