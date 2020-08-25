#pragma once

#include <QString>
#include <QVector>
#include <QtXml/QDomDocument>
#include <multimc_logic_export.h>

namespace ATLauncher
{

enum class PackType
{
    Public,
    Private
};

enum class ModType
{
    Forge,
    Jar,
    Mods,
    Flan,
    Dependency,
    Ic2Lib,
    DenLib,
    Coremods,
    MCPC,
    Plugins,
    Extract,
    Decomp,
    ResourcePack,
    Unknown
};

enum class DownloadType
{
    Server,
    Browser,
    Direct,
    Unknown
};

struct VersionPack
{
    QString version;
    QString minecraft;
};

struct VersionLoader
{
    QString type;
    QString version;
    bool latest;
    bool recommended;
};

struct VersionMod
{
    QString name;
    QString version;
    QString url;
    QString file;
    QString md5;
    DownloadType download;
    QString download_raw;
    ModType type;
    QString type_raw;
};

struct Version
{
    VersionPack pack;
    VersionLoader loader;
    QVector<VersionMod> mods;
};

MULTIMC_LOGIC_EXPORT void loadVersion(Version & v, QDomDocument & doc);

}
