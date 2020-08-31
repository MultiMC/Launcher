#pragma once

#include <QString>
#include <QVector>
#include <QJsonObject>
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

struct VersionLoader
{
    QString type;
    bool latest;
    bool recommended;
    bool choose;

    QString version;
};

struct VersionLibrary
{
    QString url;
    QString file;
    QString server;
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
    QString version;
    QString minecraft;
    bool noConfigs;
    QString mainClass;
    QString extraArguments;

    VersionLoader loader;
    QVector<VersionLibrary> libraries;
    QVector<VersionMod> mods;
};

MULTIMC_LOGIC_EXPORT void loadVersion(Version & v, QJsonObject & obj);

}
