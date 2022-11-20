#include "CFPackIndex.h"

#include "Json.h"

void CurseForge::loadIndexedPack(CurseForge::IndexedPack & pack, QJsonObject & obj)
{
    pack.addonId = Json::requireInteger(obj, "id");
    pack.name = Json::requireString(obj, "name");
    pack.summary = Json::ensureString(obj, "summary", "");

    auto logo = Json::requireObject(obj, "logo");
    pack.logoName = Json::requireString(logo, "title");
    pack.logoUrl = Json::requireString(logo, "thumbnailUrl");

    auto authors = Json::requireArray(obj, "authors");
    for(auto authorIter: authors) {
        auto author = Json::requireObject(authorIter);
        CurseForge::ModpackAuthor packAuthor;
        packAuthor.name = Json::requireString(author, "name");
        packAuthor.url = Json::requireString(author, "url");
        pack.authors.append(packAuthor);
    }
    int defaultFileId = Json::requireInteger(obj, "mainFileId");

    bool found = false;
    // check if there are some files before adding the pack
    auto files = Json::requireArray(obj, "latestFiles");
    for(auto fileIter: files) {
        auto file = Json::requireObject(fileIter);
        int id = Json::requireInteger(file, "id");

        // NOTE: for now, ignore everything that's not the default...
        if(id != defaultFileId) {
            continue;
        }

        auto versionArray = Json::requireArray(file, "gameVersions");
        if(versionArray.size() < 1) {
            continue;
        }

        found = true;
        break;
    }
    if(!found) {
        throw JSONValidationError(QString("Pack with no good file, skipping: %1").arg(pack.name));
    }

    auto links_obj = Json::ensureObject(obj, "links");
    auto loadLink = [&](const char * key) -> QString {
        auto link = Json::ensureString(links_obj, key, QString());
        if(link.endsWith('/')) {
            link.chop(1);
        }
        return link;
    };

    pack.websiteUrl = loadLink("websiteUrl");
    pack.issuesUrl = loadLink("issuesUrl");
    pack.sourceUrl = loadLink("sourceUrl");
    pack.wikiUrl = loadLink("wikiUrl");
}

void CurseForge::loadIndexedPackVersions(CurseForge::IndexedPack & pack, QJsonArray & arr)
{
    QVector<CurseForge::IndexedVersion> unsortedVersions;
    for(auto versionIter: arr) {
        auto version = Json::requireObject(versionIter);
        CurseForge::IndexedVersion file;

        file.addonId = pack.addonId;
        file.fileId = Json::requireInteger(version, "id");
        auto versionArray = Json::requireArray(version, "gameVersions");
        if(versionArray.size() < 1) {
            continue;
        }

        // pick the latest version supported
        file.mcVersion = versionArray[0].toString();
        file.version = Json::requireString(version, "displayName");
        file.downloadUrl = Json::requireString(version, "downloadUrl");
        unsortedVersions.append(file);
    }

    auto orderSortPredicate = [](const IndexedVersion & a, const IndexedVersion & b) -> bool
    {
        return a.fileId > b.fileId;
    };
    std::sort(unsortedVersions.begin(), unsortedVersions.end(), orderSortPredicate);
    pack.versions = unsortedVersions;
    pack.versionsLoaded = true;
}
