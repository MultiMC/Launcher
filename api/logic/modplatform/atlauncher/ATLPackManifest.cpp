#include "ATLPackManifest.h"

#include <QDebug>
#include <QtXml/QDomDocument>
#include <Version.h>

static void loadVersionPack(ATLauncher::VersionPack & p, const QDomElement& ele) {
    p.version = ele.firstChildElement("version").text();
    p.minecraft = ele.firstChildElement("minecraft").text();
}

static void loadVersionLoader(ATLauncher::VersionLoader & p, const QDomElement& ele) {
    p.type = ele.attribute("type");
    p.version = ele.attribute("version");
    p.latest = ele.attribute("latest") == QString("true");
    p.recommended = ele.attribute("recommended") == QString("true");
}

static void loadVersionMod(ATLauncher::VersionMod & p, const QDomElement& ele) {
    p.name = ele.attribute("name");
    p.version = ele.attribute("version");
    p.url = ele.attribute("url");
    p.file = ele.attribute("file");
    p.md5 = ele.attribute("md5");

    p.download_raw = ele.attribute("download");
    if(p.download_raw == QString("server")) {
        p.download = ATLauncher::DownloadType::Server;
    }
    else if(p.download_raw == QString("browser")) {
        p.download = ATLauncher::DownloadType::Browser;
    }
    else if(p.download_raw == QString("direct")) {
        p.download = ATLauncher::DownloadType::Direct;
    }
    else {
        p.download = ATLauncher::DownloadType::Unknown;
    }

    // See https://wiki.atlauncher.com/mod_types
    p.type_raw = ele.attribute("type");
    if(p.type_raw == QString("forge")) {
        p.type = ATLauncher::ModType::Forge;
    }
    else if(p.type_raw == QString("jar")) {
        p.type = ATLauncher::ModType::Jar;
    }
    else if(p.type_raw == QString("mods")) {
        p.type = ATLauncher::ModType::Mods;
    }
    else if(p.type_raw == QString("flan")) {
        p.type = ATLauncher::ModType::Flan;
    }
    else if(p.type_raw == QString("dependency")) {
        p.type = ATLauncher::ModType::Dependency;
    }
    else if(p.type_raw == QString("ic2lib")) {
        p.type = ATLauncher::ModType::Ic2Lib;
    }
    else if(p.type_raw == QString("denlib")) {
        p.type = ATLauncher::ModType::DenLib;
    }
    else if(p.type_raw == QString("coremods")) {
        p.type = ATLauncher::ModType::Coremods;
    }
    else if(p.type_raw == QString("mcpc")) {
        p.type = ATLauncher::ModType::MCPC;
    }
    else if(p.type_raw == QString("plugins")) {
        p.type = ATLauncher::ModType::Plugins;
    }
    else if(p.type_raw == QString("extract")) {
        p.type = ATLauncher::ModType::Extract;
    }
    else if(p.type_raw == QString("decomp")) {
        p.type = ATLauncher::ModType::Decomp;
    }
    else if(p.type_raw == QString("resourcepack")) {
        p.type = ATLauncher::ModType::ResourcePack;
    }
    else {
        p.type = ATLauncher::ModType::Unknown;
    }
}

void ATLauncher::loadVersion(Version & v, QDomDocument & doc)
{
    auto ver = doc.firstChildElement("version");

    loadVersionPack(v.pack, ver.firstChildElement("pack"));
    loadVersionLoader(v.loader, ver.firstChildElement("loader"));

    auto mods = ver.firstChildElement("mods");
    auto modList = mods.elementsByTagName("mod");
    for(int i = 0; i < modList.length(); i++)
    {
        auto modRaw = modList.at(i);
        VersionMod mod;
        loadVersionMod(mod, modRaw.toElement());
        v.mods.append(mod);
    }
}
