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

    p.type_raw = ele.attribute("type");
    if(p.type_raw == QString("mods")) {
        p.type = ATLauncher::ModType::Mods;
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
