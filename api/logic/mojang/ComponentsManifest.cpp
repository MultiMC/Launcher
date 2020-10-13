#include "ComponentsManifest.h"

#include <Json.h>
#include <QDebug>

// https://launchermeta.mojang.com/v1/products/java-runtime/2ec0cc96c44e5a76b9c8b7c39df7210883d12871/all.json

namespace mojang_files {

namespace {
void fromJson(QJsonDocument & doc, AllPlatformsManifest & out) {
    if (!doc.isObject())
    {
        throw JSONValidationError("file manifest is not an object");
    }
    QJsonObject root = doc.object();

    auto platformIter = root.begin();
    while (platformIter != root.end()) {
        QString platformName = platformIter.key();
        auto platformValue = platformIter.value();
        platformIter++;
        if (!platformValue.isObject())
        {
            throw JSONValidationError("platform entry inside manifest is not an object: " + platformName);
        }
        auto platformObject = platformValue.toObject();
        auto componentIter = platformObject.begin();
        ComponentsPlatform outPlatform;
        while (componentIter != platformObject.end()) {
            QString componentName = componentIter.key();
            auto componentValue = componentIter.value();
            componentIter++;
            if (!componentValue.isArray())
            {
                throw JSONValidationError("component version list inside manifest is not an array: " + componentName);
            }
            auto versionArray = componentValue.toArray();
            VersionList outVersionList;
            int i = 0;
            for (auto versionValue: versionArray) {
                if (!versionValue.isObject())
                {
                    throw JSONValidationError("version is not an object: " + componentName + "[" + i + "]");
                }
                i++;
                auto versionObject = versionValue.toObject();
                ComponentVersion outVersion;
                auto availaibility = Json::requireObject(versionObject, "availability");
                outVersion.availability_group = Json::requireInteger(availaibility, "group");
                outVersion.availability_progress = Json::requireInteger(availaibility, "progress");
                auto manifest = Json::requireObject(versionObject, "manifest");
                outVersion.manifest_sha1 = Json::requireString(manifest, "sha1");
                outVersion.manifest_size = Json::requireInteger(manifest, "size");
                outVersion.manifest_url = Json::requireUrl(manifest, "url");
                auto version = Json::requireObject(versionObject, "version");
                outVersion.version_name = Json::requireString(version, "name");
                outVersion.version_released = Json::requireDateTime(version, "released");
                outVersionList.versions.push_back(outVersion);
            }
            if(outVersionList.versions.size()) {
                outPlatform.components[componentName] = std::move(outVersionList);
            }
        }
        if(outPlatform.components.size()) {
            out.platforms[platformName] = outPlatform;
        }
    }
    out.valid = true;
}
}

AllPlatformsManifest AllPlatformsManifest::fromManifestContents(const QByteArray& contents)
{
    AllPlatformsManifest out;
    try
    {
        auto doc = Json::requireDocument(contents, "AllPlatformsManifest");
        fromJson(doc, out);
        return out;
    }
    catch (const Exception &e)
    {
        qDebug() << QString("Unable to parse manifest: %1").arg(e.cause());
        out.valid = false;
        return out;
    }
}

AllPlatformsManifest AllPlatformsManifest::fromManifestFile(const QString & filename) {
    AllPlatformsManifest out;
    try
    {
        auto doc = Json::requireDocument(filename, filename);
        fromJson(doc, out);
        return out;
    }
    catch (const Exception &e)
    {
        qDebug() << QString("Unable to parse manifest file %1: %2").arg(filename, e.cause());
        out.valid = false;
        return out;
    }
}

}
