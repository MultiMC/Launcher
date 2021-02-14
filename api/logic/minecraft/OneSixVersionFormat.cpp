#include "OneSixVersionFormat.h"
#include <Json.h>
#include "minecraft/ParseUtils.h"
#include <minecraft/MojangVersionFormat.h>

using namespace Json;

static void readString(const QJsonObject &root, const QString &key, QString &variable)
{
    if (root.contains(key))
    {
        variable = requireString(root.value(key));
    }
}

LibraryPtr OneSixVersionFormat::libraryFromJson(const QJsonObject &libObj, const QString &filename)
{
    LibraryPtr out = MojangVersionFormat::libraryFromJson(libObj, filename);
    readString(libObj, LAUNCHER_BUILD_NAME_SHORT + "-hint", out->m_hint);
    readString(libObj, LAUNCHER_BUILD_NAME_SHORT + "-absulute_url", out->m_absoluteURL);
    readString(libObj, LAUNCHER_BUILD_NAME_SHORT + "-absoluteUrl", out->m_absoluteURL);
    readString(libObj, LAUNCHER_BUILD_NAME_SHORT + "-filename", out->m_filename);
    readString(libObj, LAUNCHER_BUILD_NAME_SHORT + "-displayname", out->m_displayname);
    return out;
}

QJsonObject OneSixVersionFormat::libraryToJson(Library *library)
{
    QJsonObject libRoot = MojangVersionFormat::libraryToJson(library);
    if (library->m_absoluteURL.size())
        libRoot.insert(LAUNCHER_BUILD_NAME_SHORT + "-absoluteUrl", library->m_absoluteURL);
    if (library->m_hint.size())
        libRoot.insert(LAUNCHER_BUILD_NAME_SHORT + "-hint", library->m_hint);
    if (library->m_filename.size())
        libRoot.insert(LAUNCHER_BUILD_NAME_SHORT + "-filename", library->m_filename);
    if (library->m_displayname.size())
        libRoot.insert(LAUNCHER_BUILD_NAME_SHORT + "-displayname", library->m_displayname);
    return libRoot;
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

    auto readLibs = [&](const char * which, QList<LibraryPtr> & out)
    {
        for (auto libVal : requireArray(root.value(which)))
        {
            QJsonObject libObj = requireObject(libVal);
            // parse the library
            auto lib = libraryFromJson(libObj, filename);
            out.append(lib);
        }
    };
    bool hasPlusLibs = root.contains("+libraries");
    bool hasLibs = root.contains("libraries");
    if (hasPlusLibs && hasLibs)
    {
        out->addProblem(ProblemSeverity::Warning,
                        QObject::tr("Version file has both '+libraries' and 'libraries'. This is no longer supported."));
        readLibs("libraries", out->libraries);
        readLibs("+libraries", out->libraries);
    }
    else if (hasLibs)
    {
        readLibs("libraries", out->libraries);
    }
    else if(hasPlusLibs)
    {
        readLibs("+libraries", out->libraries);
    }

    if(root.contains("mavenFiles")) {
        readLibs("mavenFiles", out->mavenFiles);
    }

    // if we have mainJar, just use it
    if(root.contains("mainJar"))
    {
        QJsonObject libObj = requireObject(root, "mainJar");
        out->mainJar = libraryFromJson(libObj, filename);
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
            out->addProblem(
                ProblemSeverity::Error,
                QObject::tr("URL for the main jar could not be determined - Mojang removed the server that we used as fallback.")
            );
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
    if (!patch->mavenFiles.isEmpty())
    {
        QJsonArray array;
        for (auto value: patch->mavenFiles)
        {
            array.append(OneSixVersionFormat::libraryToJson(value.get()));
        }
        root.insert("mavenFiles", array);
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
    out->setRawName(GradleSpecifier("org.launcher.jarmods:" + id + ":1"));

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
