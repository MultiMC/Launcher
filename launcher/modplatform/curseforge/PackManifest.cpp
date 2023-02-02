#include "PackManifest.h"
#include "Json.h"

static void loadFileV1(CurseForge::File & f, QJsonObject & file)
{
    f.projectId = Json::requireInteger(file, "projectID");
    f.fileId = Json::requireInteger(file, "fileID");
    f.required = Json::ensureBoolean(file, QString("required"), true);
}

static void loadModloaderV1(CurseForge::Modloader & m, QJsonObject & modLoader)
{
    m.id = Json::requireString(modLoader, "id");
    m.primary = Json::ensureBoolean(modLoader, QString("primary"), false);
}

static void loadMinecraftV1(CurseForge::Minecraft & m, QJsonObject & minecraft)
{
    m.version = Json::requireString(minecraft, "version");
    // extra libraries... apparently only used for a custom Minecraft launcher in the 1.2.5 FTB retro pack
    // intended use is likely hardcoded in the 'CurseForge' client, the manifest says nothing
    m.libraries = Json::ensureString(minecraft, QString("libraries"), QString());
    auto arr = Json::ensureArray(minecraft, "modLoaders", QJsonArray());
    for (QJsonValueRef item : arr)
    {
        auto obj = Json::requireObject(item);
        CurseForge::Modloader loader;
        loadModloaderV1(loader, obj);
        m.modLoaders.append(loader);
    }
}

static void loadManifestV1(CurseForge::Manifest & m, QJsonObject & manifest)
{
    auto mc = Json::requireObject(manifest, "minecraft");
    loadMinecraftV1(m.minecraft, mc);
    m.name = Json::ensureString(manifest, QString("name"), "Unnamed");
    m.version = Json::ensureString(manifest, QString("version"), QString());
    m.author = Json::ensureString(manifest, QString("author"), "Anonymous Coward");
    auto arr = Json::ensureArray(manifest, "files", QJsonArray());
    for (QJsonValueRef item : arr)
    {
        auto obj = Json::requireObject(item);
        CurseForge::File file;
        loadFileV1(file, obj);
        m.files.insert(file.fileId, file);
    }
    m.overrides = Json::ensureString(manifest, "overrides", "overrides");
}

void CurseForge::loadManifest(CurseForge::Manifest & m, const QString &filepath)
{
    auto doc = Json::requireDocument(filepath);
    auto obj = Json::requireObject(doc);
    m.manifestType = Json::requireString(obj, "manifestType");
    if(m.manifestType != "minecraftModpack")
    {
        throw JSONValidationError("Not a modpack manifest!");
    }
    m.manifestVersion = Json::requireInteger(obj, "manifestVersion");
    if(m.manifestVersion != 1)
    {
        throw JSONValidationError(QString("Unknown manifest version (%1)").arg(m.manifestVersion));
    }
    loadManifestV1(m, obj);
}

bool CurseForge::File::parse(QJsonObject fileObject) {
    try {
        fileName = Json::requireString(fileObject, "fileName");
        // FIXME: this is a terrible hack
        if (fileName.endsWith(".zip")) {
            targetFolder = "resourcepacks";
        } else {
            targetFolder = "mods";
        }
        auto hashes = Json::ensureArray(fileObject, "hashes");
        for(QJsonValueRef item : hashes) {
            auto hashesObj = Json::requireObject(item);
            auto algo = Json::requireInteger(hashesObj, "algo");
            auto value = Json::requireString(hashesObj, "value");
            switch(algo) {
                case 1: {
                    sha1 = value;
                    break;
                }
                case 2: {
                    md5 = value;
                    break;
                }
                default: {
                    qWarning() << "Unknown curse hash type: " << value;
                    break;
                }
            }
        }
        QString rawUrl = Json::ensureString(fileObject, "downloadUrl");
        url = QUrl(rawUrl, QUrl::TolerantMode);
        if(!url.isEmpty()) {
            resolved = true;
        }
        return true;
    }
    catch (const Json::JsonException & error) {
        qWarning() << "Error while parsing mod from CurseForge: " << error.what();
        return false;
    }
}
