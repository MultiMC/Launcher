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
    Root,
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
    TexturePack,
    ResourcePack,
    ShaderPack,
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
    bool noConfigs;

    QString mainClass;
    QString extraArguments;
};

struct VersionLoader
{
    QString type;
    QString version;
    bool latest;
    bool recommended;
    bool choose;
};

struct VersionLibrary
{
    QString url;
    QString file;
    QString md5;
    DownloadType download;
    QString download_raw;
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

    ModType extractTo;
    QString extractTo_raw;
    QString extractFolder;

    bool optional;
};

struct Version
{
    VersionPack pack;
    VersionLoader loader;
    QVector<VersionLibrary> libraries;
    QVector<VersionMod> mods;
};

MULTIMC_LOGIC_EXPORT void loadVersion(Version & v, QDomDocument & doc);

}
