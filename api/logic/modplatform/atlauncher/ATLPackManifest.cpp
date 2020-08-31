#include "ATLPackManifest.h"

#include <QDebug>
#include <QtXml/QDomDocument>
#include <Version.h>

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
    else if(rawType == QString("dependency")) {
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
    else if(rawType == QString("resourcepack")) {
        return ATLauncher::ModType::ResourcePack;
    }

    return ATLauncher::ModType::Unknown;
}

static void loadVersionPack(ATLauncher::VersionPack & p, const QDomElement& ele) {
    p.version = ele.firstChildElement("version").text();
    p.minecraft = ele.firstChildElement("minecraft").text();
    p.noConfigs = ele.firstChildElement("noconfigs").text() == QString("true");
    p.mainClass = ele.firstChildElement("mainclass").text();
    p.extraArguments = ele.firstChildElement("extraarguments").text();
}

static void loadVersionLoader(ATLauncher::VersionLoader & p, const QDomElement& ele) {
    p.type = ele.attribute("type");
    p.version = ele.attribute("version");
    p.latest = ele.attribute("latest") == QString("true");
    p.recommended = ele.attribute("recommended") == QString("true");
    p.choose = ele.attribute("choose") == QString("true");
}

static void loadVersionLibrary(ATLauncher::VersionLibrary & p, const QDomElement& ele) {
    p.url = ele.attribute("url");
    p.file = ele.attribute("file");
    p.md5 = ele.attribute("md5");

    p.download_raw = ele.attribute("download");
    p.download = parseDownloadType(p.download_raw);
}

static void loadVersionMod(ATLauncher::VersionMod & p, const QDomElement& ele) {
    p.name = ele.attribute("name");
    p.version = ele.attribute("version");
    p.url = ele.attribute("url");
    p.file = ele.attribute("file");
    p.md5 = ele.attribute("md5");

    p.download_raw = ele.attribute("download");
    p.download = parseDownloadType(p.download_raw);

    p.type_raw = ele.attribute("type");
    p.type = parseModType(p.type_raw);

    p.extractTo_raw = ele.attribute("extractto");
    p.extractTo = parseModType(p.extractTo_raw);
    p.extractFolder = ele.attribute("extractfolder").replace("%s%", "/");

    p.optional = ele.attribute("optional") == QString("yes");
}

void ATLauncher::loadVersion(Version & v, QDomDocument & doc)
{
    auto root = doc.firstChildElement("version");

    loadVersionPack(v.pack, root.firstChildElement("pack"));
    loadVersionLoader(v.loader, root.firstChildElement("loader"));

    auto libraries = root.firstChildElement("libraries");
    auto libraryList = libraries.elementsByTagName("library");
    for(int i = 0; i < libraryList.length(); i++)
    {
        auto libraryRaw = libraryList.at(i);
        VersionLibrary library;
        loadVersionLibrary(library, libraryRaw.toElement());
        v.libraries.append(library);
    }

    auto mods = root.firstChildElement("mods");
    auto modList = mods.elementsByTagName("mod");
    for(int i = 0; i < modList.length(); i++)
    {
        auto modRaw = modList.at(i);
        VersionMod mod;
        loadVersionMod(mod, modRaw.toElement());
        v.mods.append(mod);
    }
}
