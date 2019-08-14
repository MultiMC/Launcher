#include "OneSixVersionFormat.h"
#include <Json.h>
#include "minecraft/ParseUtils.h"

class MojangVersionFormat
{
friend class OneSixVersionFormat;
protected:
    // does not include libraries
    static void readVersionProperties(const QJsonObject& in, VersionFile* out);
    // does not include libraries
    static void writeVersionProperties(const VersionFile* in, QJsonObject& out);
public:
    // libraries
    static LibraryPtr libraryFromJson(const QJsonObject &libObj, const QString &filename);
    static QJsonObject libraryToJson(Library *library);
};


using namespace Json;

static const int CURRENT_MINIMUM_LAUNCHER_VERSION = 18;

namespace
{
void readString(const QJsonObject &root, const QString &key, QString &variable)
{
    if (root.contains(key))
    {
        variable = requireString(root.value(key));
    }
}

void readDownloadInfo(MojangDownloadInfo::Ptr out, const QJsonObject &obj)
{
    // optional, not used
    readString(obj, "path", out->path);
    // required!
    out->sha1 = requireString(obj, "sha1");
    out->url = ensureString(obj, "url", QString());
    out->size = requireInteger(obj, "size");
}

void readAssetIndex(MojangAssetIndexInfo::Ptr out, const QJsonObject &obj)
{
    out->totalSize = requireInteger(obj, "totalSize");
    out->id = requireString(obj, "id");
    // out->known = true;
}

MojangAssetIndexInfo::Ptr assetIndexFromJson(const QJsonObject &obj)
{
    auto out = std::make_shared<MojangAssetIndexInfo>();
    ::readDownloadInfo(out, obj);
    ::readAssetIndex(out, obj);
    return out;
}


QJsonObject assetIndexToJson(MojangAssetIndexInfo::Ptr info)
{
    QJsonObject out;
    if(!info->path.isNull())
    {
        out.insert("path", info->path);
    }
    out.insert("sha1", info->sha1);
    out.insert("size", info->size);
    out.insert("url", info->url);
    out.insert("totalSize", info->totalSize);
    out.insert("id", info->id);
    return out;
}

MojangDownloadInfo::Ptr downloadInfoFromJson(const QJsonObject &obj)
{
    auto out = std::make_shared<MojangDownloadInfo>();
    ::readDownloadInfo(out, obj);
    return out;
}

QJsonObject downloadInfoToJson(MojangDownloadInfo::Ptr info)
{
    QJsonObject out;
    if(!info->path.isNull())
    {
        out.insert("path", info->path);
    }
    out.insert("sha1", info->sha1);
    out.insert("size", info->size);
    out.insert("url", info->url);
    return out;
}

MojangLibraryDownloadInfo::Ptr libDownloadInfoFromJson(const QJsonObject &libObj)
{
    auto out = std::make_shared<MojangLibraryDownloadInfo>();
    auto dlObj = requireObject(libObj.value("downloads"));
    if(dlObj.contains("artifact"))
    {
        out->artifact = downloadInfoFromJson(requireObject(dlObj, "artifact"));
    }
    if(dlObj.contains("classifiers"))
    {
        auto classifiersObj = requireObject(dlObj, "classifiers");
        for(auto iter = classifiersObj.begin(); iter != classifiersObj.end(); iter++)
        {
            auto classifier = iter.key();
            auto classifierObj = requireObject(iter.value());
            out->classifiers[classifier] = downloadInfoFromJson(classifierObj);
        }
    }
    return out;
}



}

LibraryPtr OneSixVersionFormat::libraryFromJson(const QJsonObject &libObj, const QString &filename)
{
    LibraryPtr out(new Library());
    if (!libObj.contains("name"))
    {
        throw JSONValidationError(filename + "contains a library that doesn't have a 'name' field");
    }
    out->m_name = libObj.value("name").toString();

    ::readString(libObj, "url", out->m_repositoryURL);
    if (libObj.contains("extract"))
    {
        out->m_hasExcludes = true;
        auto extractObj = requireObject(libObj.value("extract"));
        for (auto excludeVal : requireArray(extractObj.value("exclude")))
        {
            out->m_extractExcludes.append(requireString(excludeVal));
        }
    }
    if (libObj.contains("natives"))
    {
        QJsonObject nativesObj = requireObject(libObj.value("natives"));
        for (auto it = nativesObj.begin(); it != nativesObj.end(); ++it)
        {
            if (!it.value().isString())
            {
                qWarning() << filename << "contains an invalid native (skipping)";
            }
            OpSys opSys = OpSys_fromString(it.key());
            if (opSys != Os_Other)
            {
                out->m_nativeClassifiers[opSys] = it.value().toString();
            }
        }
    }
    if (libObj.contains("rules"))
    {
        out->applyRules = true;
        out->m_rules = rulesFromJsonV4(libObj);
    }
    if (libObj.contains("downloads"))
    {
        out->m_mojangDownloads = libDownloadInfoFromJson(libObj);
    }

    readString(libObj, "MMC-hint", out->m_hint);
    readString(libObj, "MMC-absulute_url", out->m_absoluteURL);
    readString(libObj, "MMC-absoluteUrl", out->m_absoluteURL);
    readString(libObj, "MMC-filename", out->m_filename);
    readString(libObj, "MMC-displayname", out->m_displayname);
    return out;
}

QJsonObject libDownloadInfoToJson(MojangLibraryDownloadInfo::Ptr libinfo)
{
    QJsonObject out;
    if(libinfo->artifact)
    {
        out.insert("artifact", downloadInfoToJson(libinfo->artifact));
    }
    if(libinfo->classifiers.size())
    {
        QJsonObject classifiersOut;
        for(auto iter = libinfo->classifiers.begin(); iter != libinfo->classifiers.end(); iter++)
        {
            classifiersOut.insert(iter.key(), downloadInfoToJson(iter.value()));
        }
        out.insert("classifiers", classifiersOut);
    }
    return out;
}

QJsonObject OneSixVersionFormat::libraryToJson(Library *library)
{
    QJsonObject libRoot;
    libRoot.insert("name", (QString)library->m_name);
    if (!library->m_repositoryURL.isEmpty())
    {
        libRoot.insert("url", library->m_repositoryURL);
    }
    if (library->isNative())
    {
        QJsonObject nativeList;
        auto iter = library->m_nativeClassifiers.begin();
        while (iter != library->m_nativeClassifiers.end())
        {
            nativeList.insert(OpSys_toString(iter.key()), iter.value());
            iter++;
        }
        libRoot.insert("natives", nativeList);
        if (library->m_extractExcludes.size())
        {
            QJsonArray excludes;
            QJsonObject extract;
            for (auto exclude : library->m_extractExcludes)
            {
                excludes.append(exclude);
            }
            extract.insert("exclude", excludes);
            libRoot.insert("extract", extract);
        }
    }
    if (library->m_rules.size())
    {
        QJsonArray allRules;
        for (auto &rule : library->m_rules)
        {
            QJsonObject ruleObj = rule->toJson();
            allRules.append(ruleObj);
        }
        libRoot.insert("rules", allRules);
    }
    if(library->m_mojangDownloads)
    {
        auto downloadsObj = libDownloadInfoToJson(library->m_mojangDownloads);
        libRoot.insert("downloads", downloadsObj);
    }

    // MultiMC extensions
    if (library->m_absoluteURL.size()) {
        libRoot.insert("MMC-absoluteUrl", library->m_absoluteURL);
    }
    if (library->m_hint.size()) {
        libRoot.insert("MMC-hint", library->m_hint);
    }
    if (library->m_filename.size()) {
        libRoot.insert("MMC-filename", library->m_filename);
    }
    if (library->m_displayname.size()) {
        libRoot.insert("MMC-displayname", library->m_displayname);
    }

    return libRoot;
}

void MojangVersionFormat::readVersionProperties(const QJsonObject &in, VersionFile *out)
{
    ::readString(in, "id", out->minecraftVersion);
    ::readString(in, "mainClass", out->mainClass);
    ::readString(in, "minecraftArguments", out->minecraftArguments);
    if(out->minecraftArguments.isEmpty())
    {
        QString processArguments;
        ::readString(in, "processArguments", processArguments);
        QString toCompare = processArguments.toLower();
        if (toCompare == "legacy")
        {
            out->minecraftArguments = " ${auth_player_name} ${auth_session}";
        }
        else if (toCompare == "username_session")
        {
            out->minecraftArguments = "--username ${auth_player_name} --session ${auth_session}";
        }
        else if (toCompare == "username_session_version")
        {
            out->minecraftArguments = "--username ${auth_player_name} --session ${auth_session} --version ${profile_name}";
        }
        else if (!toCompare.isEmpty())
        {
            out->addProblem(ProblemSeverity::Error, QObject::tr("processArguments is set to unknown value '%1'").arg(processArguments));
        }
    }
    ::readString(in, "type", out->type);

    ::readString(in, "assets", out->assets);
    if(in.contains("assetIndex"))
    {
        out->mojangAssetIndex = assetIndexFromJson(requireObject(in, "assetIndex"));
    }
    else if (!out->assets.isNull())
    {
        out->mojangAssetIndex = std::make_shared<MojangAssetIndexInfo>(out->assets);
    }

    out->releaseTime = timeFromS3Time(in.value("releaseTime").toString(""));
    out->updateTime = timeFromS3Time(in.value("time").toString(""));

    if (in.contains("minimumLauncherVersion"))
    {
        out->minimumLauncherVersion = requireInteger(in.value("minimumLauncherVersion"));
        if (out->minimumLauncherVersion > CURRENT_MINIMUM_LAUNCHER_VERSION)
        {
            out->addProblem(
                ProblemSeverity::Warning,
                QObject::tr("The 'minimumLauncherVersion' value of this version (%1) is higher than supported by MultiMC (%2). It might not work properly!")
                    .arg(out->minimumLauncherVersion)
                    .arg(CURRENT_MINIMUM_LAUNCHER_VERSION)
            );
        }
    }
    if(in.contains("downloads"))
    {
        auto downloadsObj = requireObject(in, "downloads");
        for(auto iter = downloadsObj.begin(); iter != downloadsObj.end(); iter++)
        {
            auto classifier = iter.key();
            auto classifierObj = requireObject(iter.value());
            out->mojangDownloads[classifier] = downloadInfoFromJson(classifierObj);
        }
    }
}

VersionFilePtr OneSixVersionFormat::versionFileFromJson(const QJsonDocument &doc, const QString &filename, const bool requireOrder)
{
    VersionFilePtr out(new VersionFile());
    if (doc.isEmpty() || doc.isNull())
    {
        throw JSONValidationError(filename + " is empty or null");
    }
    if (!doc.isObject())
    {
        throw JSONValidationError(filename + " is not an object");
    }

    QJsonObject root = doc.object();

    Meta::MetadataVersion formatVersion = Meta::parseFormatVersion(root, false);
    switch(formatVersion)
    {
        case Meta::MetadataVersion::InitialRelease:
            break;
        case Meta::MetadataVersion::Invalid:
            throw JSONValidationError(filename + " does not contain a recognizable version of the metadata format.");
    }

    if (requireOrder)
    {
        if (root.contains("order"))
        {
            out->order = requireInteger(root.value("order"));
        }
        else
        {
            // FIXME: evaluate if we don't want to throw exceptions here instead
            qCritical() << filename << "doesn't contain an order field";
        }
    }

    out->name = root.value("name").toString();

    if(root.contains("uid"))
    {
        out->uid = root.value("uid").toString();
    }
    else
    {
        out->uid = root.value("fileId").toString();
    }

    out->version = root.value("version").toString();

    MojangVersionFormat::readVersionProperties(root, out.get());

    // added for legacy Minecraft window embedding, TODO: remove
    readString(root, "appletClass", out->appletClass);

    if (root.contains("+tweakers"))
    {
        for (auto tweakerVal : requireArray(root.value("+tweakers")))
        {
            out->addTweakers.append(requireString(tweakerVal));
        }
    }

    if (root.contains("+traits"))
    {
        for (auto tweakerVal : requireArray(root.value("+traits")))
        {
            out->traits.insert(requireString(tweakerVal));
        }
    }


    if (root.contains("jarMods"))
    {
        for (auto libVal : requireArray(root.value("jarMods")))
        {
            QJsonObject libObj = requireObject(libVal);
            // parse the jarmod
            auto lib = OneSixVersionFormat::jarModFromJson(libObj, filename);
            // and add to jar mods
            out->jarMods.append(lib);
        }
    }
    else if (root.contains("+jarMods")) // DEPRECATED: old style '+jarMods' are only here for backwards compatibility
    {
        for (auto libVal : requireArray(root.value("+jarMods")))
        {
            QJsonObject libObj = requireObject(libVal);
            // parse the jarmod
            auto lib = OneSixVersionFormat::plusJarModFromJson(libObj, filename, out->name);
            // and add to jar mods
            out->jarMods.append(lib);
        }
    }

    if (root.contains("mods"))
    {
        for (auto libVal : requireArray(root.value("mods")))
        {
            QJsonObject libObj = requireObject(libVal);
            // parse the jarmod
            auto lib = OneSixVersionFormat::modFromJson(libObj, filename);
            // and add to jar mods
            out->mods.append(lib);
        }
    }

    auto readLibs = [&](const char * which)
    {
        for (auto libVal : requireArray(root.value(which)))
        {
            QJsonObject libObj = requireObject(libVal);
            // parse the library
            auto lib = libraryFromJson(libObj, filename);
            out->libraries.append(lib);
        }
    };
    bool hasPlusLibs = root.contains("+libraries");
    bool hasLibs = root.contains("libraries");
    if (hasPlusLibs && hasLibs)
    {
        out->addProblem(ProblemSeverity::Warning,
                        QObject::tr("Version file has both '+libraries' and 'libraries'. This is no longer supported."));
        readLibs("libraries");
        readLibs("+libraries");
    }
    else if (hasLibs)
    {
        readLibs("libraries");
    }
    else if(hasPlusLibs)
    {
        readLibs("+libraries");
    }

    // if we have mainJar, just use it
    if(root.contains("mainJar"))
    {
        auto val = root.value("mainJar");
        if(val.isNull())
        {
            out->removeMainJar = true;
        }
        else
        {
            QJsonObject libObj = requireObject(root, "mainJar");
            out->mainJar = libraryFromJson(libObj, filename);
        }
    }
    // else reconstruct it from downloads and id ... if that's available
    else if(!out->minecraftVersion.isEmpty())
    {
        auto lib = std::make_shared<Library>();
        lib->setRawName(GradleSpecifier(QString("com.mojang:minecraft:%1:client").arg(out->minecraftVersion)));
        // we have a reliable client download, use it.
        if(out->mojangDownloads.contains("client"))
        {
            auto LibDLInfo = std::make_shared<MojangLibraryDownloadInfo>();
            LibDLInfo->artifact = out->mojangDownloads["client"];
            lib->setMojangDownloadInfo(LibDLInfo);
        }
        // we got nothing... guess based on ancient hardcoded Mojang behaviour
        // FIXME: this will eventually break...
        else
        {
            lib->setAbsoluteUrl(URLConstants::getLegacyJarUrl(out->minecraftVersion));
        }
        out->mainJar = lib;
    }

    if (root.contains("requires"))
    {
        Meta::parseRequires(root, &out->requires);
    }
    QString dependsOnMinecraftVersion = root.value("mcVersion").toString();
    if(!dependsOnMinecraftVersion.isEmpty())
    {
        Meta::Require mcReq;
        mcReq.uid = "net.minecraft";
        mcReq.equalsVersion = dependsOnMinecraftVersion;
        if (out->requires.count(mcReq) == 0)
        {
            out->requires.insert(mcReq);
        }
    }
    if (root.contains("conflicts"))
    {
        Meta::parseRequires(root, &out->conflicts);
    }
    if (root.contains("volatile"))
    {
        out->m_volatile = requireBoolean(root, "volatile");
    }

    /* removed features that shouldn't be used */
    if (root.contains("tweakers"))
    {
        out->addProblem(ProblemSeverity::Error, QObject::tr("Version file contains unsupported element 'tweakers'"));
    }
    if (root.contains("-libraries"))
    {
        out->addProblem(ProblemSeverity::Error, QObject::tr("Version file contains unsupported element '-libraries'"));
    }
    if (root.contains("-tweakers"))
    {
        out->addProblem(ProblemSeverity::Error, QObject::tr("Version file contains unsupported element '-tweakers'"));
    }
    if (root.contains("-minecraftArguments"))
    {
        out->addProblem(ProblemSeverity::Error, QObject::tr("Version file contains unsupported element '-minecraftArguments'"));
    }
    if (root.contains("+minecraftArguments"))
    {
        out->addProblem(ProblemSeverity::Error, QObject::tr("Version file contains unsupported element '+minecraftArguments'"));
    }
    return out;
}

void MojangVersionFormat::writeVersionProperties(const VersionFile* in, QJsonObject& out)
{
    writeString(out, "id", in->minecraftVersion);
    writeString(out, "mainClass", in->mainClass);
    writeString(out, "minecraftArguments", in->minecraftArguments);
    writeString(out, "type", in->type);
    if(!in->releaseTime.isNull())
    {
        writeString(out, "releaseTime", timeToS3Time(in->releaseTime));
    }
    if(!in->updateTime.isNull())
    {
        writeString(out, "time", timeToS3Time(in->updateTime));
    }
    if(in->minimumLauncherVersion != -1)
    {
        out.insert("minimumLauncherVersion", in->minimumLauncherVersion);
    }
    writeString(out, "assets", in->assets);
    if(in->mojangAssetIndex && in->mojangAssetIndex->known)
    {
        out.insert("assetIndex", assetIndexToJson(in->mojangAssetIndex));
    }
    if(in->mojangDownloads.size())
    {
        QJsonObject downloadsOut;
        for(auto iter = in->mojangDownloads.begin(); iter != in->mojangDownloads.end(); iter++)
        {
            downloadsOut.insert(iter.key(), downloadInfoToJson(iter.value()));
        }
        out.insert("downloads", downloadsOut);
    }
}

QJsonDocument OneSixVersionFormat::versionFileToJson(const VersionFilePtr &patch)
{
    QJsonObject root;
    writeString(root, "name", patch->name);

    writeString(root, "uid", patch->uid);

    writeString(root, "version", patch->version);

    Meta::serializeFormatVersion(root, Meta::MetadataVersion::InitialRelease);

    MojangVersionFormat::writeVersionProperties(patch.get(), root);

    if(patch->mainJar)
    {
        root.insert("mainJar", libraryToJson(patch->mainJar.get()));
    }
    else if(patch->removeMainJar)
    {
        root.insert("mainJar", QJsonValue());
    }
    writeString(root, "appletClass", patch->appletClass);
    writeStringList(root, "+tweakers", patch->addTweakers);
    writeStringList(root, "+traits", patch->traits.toList());
    if (!patch->libraries.isEmpty())
    {
        QJsonArray array;
        for (auto value: patch->libraries)
        {
            array.append(OneSixVersionFormat::libraryToJson(value.get()));
        }
        root.insert("libraries", array);
    }
    if (!patch->jarMods.isEmpty())
    {
        QJsonArray array;
        for (auto value: patch->jarMods)
        {
            array.append(OneSixVersionFormat::jarModtoJson(value.get()));
        }
        root.insert("jarMods", array);
    }
    if (!patch->mods.isEmpty())
    {
        QJsonArray array;
        for (auto value: patch->jarMods)
        {
            array.append(OneSixVersionFormat::modtoJson(value.get()));
        }
        root.insert("mods", array);
    }
    if(!patch->requires.empty())
    {
        Meta::serializeRequires(root, &patch->requires, "requires");
    }
    if(!patch->conflicts.empty())
    {
        Meta::serializeRequires(root, &patch->conflicts, "conflicts");
    }
    if(patch->m_volatile)
    {
        root.insert("volatile", true);
    }
    // write the contents to a json document.
    {
        QJsonDocument out;
        out.setObject(root);
        return out;
    }
}

LibraryPtr OneSixVersionFormat::plusJarModFromJson(const QJsonObject &libObj, const QString &filename, const QString &originalName)
{
    LibraryPtr out(new Library());
    if (!libObj.contains("name"))
    {
        throw JSONValidationError(filename +
                                  "contains a jarmod that doesn't have a 'name' field");
    }

    // just make up something unique on the spot for the library name.
    auto uuid = QUuid::createUuid();
    QString id = uuid.toString().remove('{').remove('}');
    out->setRawName(GradleSpecifier("org.multimc.jarmods:" + id + ":1"));

    // filename override is the old name
    out->setFilename(libObj.value("name").toString());

    // it needs to be local, it is stored in the instance jarmods folder
    out->setHint("local");

    // read the original name if present - some versions did not set it
    // it is the original jar mod filename before it got renamed at the point of addition
    auto displayName = libObj.value("originalName").toString();
    if(displayName.isEmpty())
    {
        auto fixed = originalName;
        fixed.remove(" (jar mod)");
        out->setDisplayName(fixed);
    }
    else
    {
        out->setDisplayName(displayName);
    }
    return out;
}

LibraryPtr OneSixVersionFormat::jarModFromJson(const QJsonObject& libObj, const QString& filename)
{
    return libraryFromJson(libObj, filename);
}


QJsonObject OneSixVersionFormat::jarModtoJson(Library *jarmod)
{
    return libraryToJson(jarmod);
}

LibraryPtr OneSixVersionFormat::modFromJson(const QJsonObject& libObj, const QString& filename)
{
    return  libraryFromJson(libObj, filename);
}

QJsonObject OneSixVersionFormat::modtoJson(Library *jarmod)
{
    return libraryToJson(jarmod);
}
