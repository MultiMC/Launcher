#include "ATLPackManifest.h"

#include "Json.h"

static ATLauncher::DownloadType parseDownloadType(QString rawType) {
    if(rawType == QString("server")) {
        return ATLauncher::DownloadType::Server;
    }
    else if(rawType == QString("browser")) {
        return ATLauncher::DownloadType::Browser;
    }
    else if(rawType == QString("direct")) {
        return ATLauncher::DownloadType::Direct;
    }

    return ATLauncher::DownloadType::Unknown;
}

static ATLauncher::ModType parseModType(QString rawType) {
    // See https://wiki.atlauncher.com/mod_types
    if(rawType == QString("root")) {
        return ATLauncher::ModType::Root;
    }
    else if(rawType == QString("forge")) {
        return ATLauncher::ModType::Forge;
    }
    else if(rawType == QString("jar")) {
        return ATLauncher::ModType::Jar;
    }
    else if(rawType == QString("mods")) {
        return ATLauncher::ModType::Mods;
    }
    else if(rawType == QString("flan")) {
        return ATLauncher::ModType::Flan;
    }
    else if(rawType == QString("dependency") || rawType == QString("depandency")) {
        return ATLauncher::ModType::Dependency;
    }
    else if(rawType == QString("ic2lib")) {
        return ATLauncher::ModType::Ic2Lib;
    }
    else if(rawType == QString("denlib")) {
        return ATLauncher::ModType::DenLib;
    }
    else if(rawType == QString("coremods")) {
        return ATLauncher::ModType::Coremods;
    }
    else if(rawType == QString("mcpc")) {
        return ATLauncher::ModType::MCPC;
    }
    else if(rawType == QString("plugins")) {
        return ATLauncher::ModType::Plugins;
    }
    else if(rawType == QString("extract")) {
        return ATLauncher::ModType::Extract;
    }
    else if(rawType == QString("decomp")) {
        return ATLauncher::ModType::Decomp;
    }
    else if(rawType == QString("texturepack")) {
        return ATLauncher::ModType::TexturePack;
    }
    else if(rawType == QString("resourcepack")) {
        return ATLauncher::ModType::ResourcePack;
    }
    else if(rawType == QString("shaderpack")) {
        return ATLauncher::ModType::ShaderPack;
    }

    return ATLauncher::ModType::Unknown;
}

static void loadVersionLoader(ATLauncher::VersionLoader & p, QJsonObject & obj) {
    p.type = Json::requireString(obj, "type");
    p.latest = Json::ensureBoolean(obj, "latest", false);
    p.choose = Json::ensureBoolean(obj, "choose", false);
    p.recommended = Json::ensureBoolean(obj, "recommended", false);

    auto metadata = Json::requireObject(obj, "metadata");
    p.version = Json::requireString(metadata, "version");
}

static void loadVersionLibrary(ATLauncher::VersionLibrary & p, QJsonObject & obj) {
    p.url = Json::requireString(obj, "url");
    p.file = Json::requireString(obj, "file");
    p.md5 = Json::requireString(obj, "md5");

    p.download_raw = Json::requireString(obj, "download");
    p.download = parseDownloadType(p.download_raw);
}

static void loadVersionMod(ATLauncher::VersionMod & p, QJsonObject & obj) {
    p.name = Json::requireString(obj, "name");
    p.version = Json::requireString(obj, "version");
    p.url = Json::requireString(obj, "url");
    p.file = Json::requireString(obj, "file");
    p.md5 = Json::requireString(obj, "md5");

    p.download_raw = Json::requireString(obj, "download");
    p.download = parseDownloadType(p.download_raw);

    p.type_raw = Json::requireString(obj, "type");
    p.type = parseModType(p.type_raw);

    if(obj.contains("extractTo")) {
        p.extractTo_raw = Json::requireString(obj, "extractTo");
        p.extractTo = parseModType(p.extractTo_raw);
        p.extractFolder = Json::ensureString(obj, "extractFolder", "").replace("%s%", "/");
    }

    p.optional = Json::ensureBoolean(obj, "optional", false);
}

void ATLauncher::loadVersion(Version & v, QJsonObject & obj)
{
    v.version = Json::requireString(obj, "version");
    v.minecraft = Json::requireString(obj, "minecraft");
    v.noConfigs = Json::ensureBoolean(obj, "noConfigs", false);

    if(obj.contains("mainClass")) {
        auto main = Json::requireObject(obj, "mainClass");
        v.mainClass = Json::ensureString(main, "mainClass", "");
    }

    if(obj.contains("extraArguments")) {
        auto arguments = Json::requireObject(obj, "extraArguments");
        v.extraArguments = Json::ensureString(arguments, "arguments", "");
    }

    if(obj.contains("loader")) {
        auto loader = Json::requireObject(obj, "loader");
        loadVersionLoader(v.loader, loader);
    }

    if(obj.contains("libraries")) {
        auto libraries = Json::requireArray(obj, "libraries");
        for (const auto libraryRaw : libraries)
        {
            auto libraryObj = Json::requireObject(libraryRaw);
            ATLauncher::VersionLibrary target;
            loadVersionLibrary(target, libraryObj);
            v.libraries.append(target);
        }
    }

    auto mods = Json::requireArray(obj, "mods");
    for (const auto modRaw : mods)
    {
        auto modObj = Json::requireObject(modRaw);
        ATLauncher::VersionMod mod;
        loadVersionMod(mod, modObj);
        v.mods.append(mod);
    }
}
