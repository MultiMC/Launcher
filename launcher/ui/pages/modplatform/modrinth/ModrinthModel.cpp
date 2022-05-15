/*
 * Copyright 2013-2022 MultiMC Contributors
 * Copyright 2022 kb1000
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "ModrinthModel.h"
#include "Application.h"
#include "Json.h"

#include <QIcon>

Modrinth::ListModel::ListModel(QObject *parent) : QAbstractListModel(parent)
{
}

Modrinth::ListModel::~ListModel() = default;

QVariant Modrinth::ListModel::data(const QModelIndex &index, int role) const
{
    int pos = index.row();
    if(pos >= modpacks.size() || pos < 0 || !index.isValid())
    {
        return QString("INVALID INDEX %1").arg(pos);
    }

    auto pack = modpacks.at(pos);
    if(role == Qt::DisplayRole)
    {
        return pack.name;
    }
    else if(role == Qt::DecorationRole)
    {
        if(m_logoMap.contains(pack.id))
        {
            return (m_logoMap.value(pack.id));
        }
        QIcon icon = APPLICATION->getThemedIcon("screenshot-placeholder");
        ((ListModel *)this)->requestLogo(pack.id, pack.iconUrl);
        return icon;
    }
    else if (role == Qt::ToolTipRole)
    {
        return pack.description;
    }
    else if(role == Qt::UserRole)
    {
        QVariant v;
        v.setValue(pack);
        return v;
    }
    return QVariant();
}

bool Modrinth::ListModel::canFetchMore(const QModelIndex& parent) const
{
    return searchState == CanPossiblyFetchMore;
}


void Modrinth::ListModel::fetchMore(const QModelIndex& parent)
{
    if (parent.isValid())
        return;
    if(nextSearchOffset == 0) {
        qWarning() << "fetchMore with 0 offset is wrong...";
        return;
    }
    performPaginatedSearch();
}

int Modrinth::ListModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

int Modrinth::ListModel::rowCount(const QModelIndex &parent) const
{
    return modpacks.size();
}

void Modrinth::ListModel::searchWithTerm(const QString& term, const QString &sort)
{
    if(currentSearchTerm == term && currentSearchTerm.isNull() == term.isNull() && currentSort == sort) {
        return;
    }
    currentSearchTerm = term;
    currentSort = sort;
    if(jobPtr) {
        jobPtr->abort();
        searchState = ResetRequested;
        return;
    }
    else {
        beginResetModel();
        modpacks.clear();
        endResetModel();
        searchState = None;
    }
    nextSearchOffset = 0;
    performPaginatedSearch();
}

void Modrinth::ListModel::performPaginatedSearch()
{
    auto *netJob = new NetJob("Modrinth::Search", APPLICATION->network());
    QString searchUrl = "";
    if (currentSearchTerm.isEmpty()) {
        searchUrl = QString("https://api.modrinth.com/v2/search?facets=[[%22project_type:modpack%22]]&index=%1&limit=25&offset=%2").arg(currentSort).arg(nextSearchOffset);
    }
    else
    {
        searchUrl = QString(
                "https://api.modrinth.com/v2/search?facets=[[%22project_type:modpack%22]]&index=%1&limit=25&offset=%2&query=%3"
        ).arg(currentSort).arg(nextSearchOffset).arg(currentSearchTerm);
    }
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();
    QObject::connect(netJob, &NetJob::succeeded, this, &ListModel::searchRequestFinished);
    QObject::connect(netJob, &NetJob::failed, this, &ListModel::searchRequestFailed);
}

void Modrinth::ListModel::searchRequestFinished()
{
    jobPtr.reset();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError)
    {
        qWarning() << "Error while parsing JSON response from Modrinth at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    QVector<Modrinth::Modpack> newList;
    QJsonArray hits;
    int total_hits = 0;

    try
    {
        auto obj = Json::requireObject(doc);
        hits = Json::requireArray(obj, "hits");
        total_hits = Json::requireInteger(obj, "total_hits");
    }
    catch(const JSONValidationError &e)
    {
        qWarning() << "Error while parsing response from Modrinth: " << e.cause();
        return;
    }

    for (auto packRaw : hits)
    {
        auto packObj = packRaw.toObject();
        Modrinth::Modpack pack;
        try
        {
            if (Json::ensureString(packObj, "client_side", "required") == QStringLiteral("unsupported"))
                continue;
            pack.id = Json::requireString(packObj, "project_id");
            pack.name = Json::requireString(packObj, "title");
            pack.iconUrl = Json::requireUrl(packObj, "icon_url");
            pack.author = Json::requireString(packObj, "author");
            pack.description = Json::requireString(packObj, "description");
            newList.append(pack);
        }
        catch(const JSONValidationError &e)
        {
            qWarning() << "Error while loading pack from Modrinth: " << e.cause();
            continue;
        }
    }

    if ((total_hits - nextSearchOffset) <= 25)
        searchState = Finished;
    else
    {
        nextSearchOffset += 25;
        searchState = CanPossiblyFetchMore;
    }
    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + newList.size() - 1);
    // TODO: when we update from Qt 5.4, just use append(QVector)
    for(auto item: newList) {
        modpacks.append(item);
    }
    endInsertRows();
}

void Modrinth::ListModel::searchRequestFailed()
{
    jobPtr.reset();

    if(searchState == ResetRequested)
    {
        beginResetModel();
        modpacks.clear();
        endResetModel();

        nextSearchOffset = 0;
        performPaginatedSearch();
    }
    else
    {
        searchState = Finished;
    }
}

void Modrinth::ListModel::logoLoaded(const QString &logo, const QIcon &out)
{
    m_loadingLogos.removeAll(logo);
    m_logoMap.insert(logo, out);
    for(int i = 0; i < modpacks.size(); i++) {
        if(modpacks[i].id == logo) {
            emit dataChanged(createIndex(i, 0), createIndex(i, 0), {Qt::DecorationRole});
        }
    }
}

void Modrinth::ListModel::logoFailed(const QString &logo)
{
    m_failedLogos.append(logo);
    m_loadingLogos.removeAll(logo);
}

void Modrinth::ListModel::requestLogo(const QString &logo, const QUrl &url)
{
    if(m_loadingLogos.contains(logo) || m_failedLogos.contains(logo))
    {
        return;
    }

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry("ModrinthPacks", QString("logos/%1").arg(logo.section(".", 0, 0)));
    auto *job = new NetJob(QString("Modrinth Icon Download %1").arg(logo), APPLICATION->network());
    job->addNetAction(Net::Download::makeCached(url, entry));

    auto fullPath = entry->getFullPath();
    QObject::connect(job, &NetJob::succeeded, this, [this, logo, fullPath]
    {
        QIcon icon(fullPath);
        QSize size = icon.actualSize(QSize(48, 48));
        if (size.width() < 48 && size.height() < 48)
        {
            icon = icon.pixmap(48, 48).scaled(48,48, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        }
        logoLoaded(logo, icon);
        if(waitingCallbacks.contains(logo))
        {
            waitingCallbacks.value(logo)(fullPath);
        }
    });

    QObject::connect(job, &NetJob::failed, this, [this, logo]
    {
        logoFailed(logo);
    });

    job->start();

    m_loadingLogos.append(logo);
}

void Modrinth::ListModel::getPackDetails(const QString& id)
{
    auto index = getIndexFromId(id);
    if(!index) {
        return;
    }

    if(isPackDetailInProgress()) {
        queuedPackDetailRequest = id;
        cancelPackDetail();
        return;
    }

    currentPackDetailRequest = id;

    QString detailsUrl = "https://api.modrinth.com/v2/project/" + id;

    auto & modpack = modpacks[*index];
    if(modpack.detailsLoaded != LoadState::Loaded)
    {
        auto *netJob = new NetJob("Modrinth::PackDetails", APPLICATION->network());
        netJob->addNetAction(Net::Download::makeByteArray(QUrl(detailsUrl), &detailsResponse));
        detailsPtr = netJob;
        detailsPtr->start();
        QObject::connect(netJob, &NetJob::succeeded, this, &ListModel::detailsRequestFinished);
        QObject::connect(netJob, &NetJob::failed, this, &ListModel::detailsRequestFailed);
    }

    QString versionsUrl = detailsUrl + "/version";
    if(modpack.versionsLoaded != LoadState::Loaded)
    {
        auto *netJob = new NetJob("Modrinth::PackVersions", APPLICATION->network());
        netJob->addNetAction(Net::Download::makeByteArray(QUrl(versionsUrl), &versionsResponse));
        versionsPtr = netJob;
        versionsPtr->start();
        QObject::connect(netJob, &NetJob::succeeded, this, &ListModel::versionsRequestFinished);
        QObject::connect(netJob, &NetJob::failed, this, &ListModel::versionsRequestFailed);
    }
}

bool Modrinth::ListModel::isPackDetailInProgress()
{
    return detailsPtr || versionsPtr;
}

void Modrinth::ListModel::cancelPackDetail()
{
    if(detailsPtr) {
        detailsPtr->abort();
    }
    if(versionsPtr) {
        versionsPtr->abort();
    }
}

nonstd::optional<int> Modrinth::ListModel::getIndexFromId(const QString& id)
{
    for(int i = 0; i < modpacks.size(); i++) {
        if(modpacks[i].id == id) {
            return i;
        }
    }
    return nonstd::nullopt;
}

nonstd::optional<Modrinth::Modpack> Modrinth::ListModel::getModpackById(const QString& id)
{
    auto index = getIndexFromId(id);
    if(!index) {
        return nonstd::nullopt;
    }
    return modpacks[*index];
}

namespace {
bool parseDetailsInto(QByteArray & input, Modrinth::Modpack& output) {
    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(input, &parse_error);
    if(parse_error.error != QJsonParseError::NoError)
    {
        qWarning() << "Error while parsing pack details response from Modrinth at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << input;
        return false;
    }

    try
    {
        auto obj = Json::requireObject(doc);
        QString body = Json::requireString(obj, "body");
        output.body = body;
        return true;
    }
    catch(const JSONValidationError &e)
    {
        qWarning() << "Error while parsing response from Modrinth: " << e.cause();
        return false;
    }
}
}

void Modrinth::ListModel::detailsRequestFinished()
{
    auto index = getIndexFromId(currentPackDetailRequest);
    if(index) {
        auto & modpack = modpacks[*index];

        if(parseDetailsInto(detailsResponse, modpack)) {
            modpack.detailsLoaded = LoadState::Loaded;
        }
        else {
            modpack.detailsLoaded = LoadState::Errored;
        }
        emit packDataChanged(currentPackDetailRequest);
    }
    detailsPtr.reset();
    checkDetailsDone();
}

void Modrinth::ListModel::detailsRequestFailed()
{
    auto index = getIndexFromId(currentPackDetailRequest);
    if(index) {
        auto & modpack = modpacks[*index];
        if(modpack.detailsLoaded == LoadState::NotLoaded) {
            modpack.detailsLoaded = LoadState::Errored;
            emit packDataChanged(currentPackDetailRequest);
        }
    }
    detailsPtr.reset();
    checkDetailsDone();
}

/*
  {
    "id": "8mMRnfwS",
    "project_id": "WCJmvhgU",
    "author_id": "akScBBW1",
    "featured": true,
    "name": "Third Release",
    "version_number": "2022.1.12",
    "changelog": "This is the third release!",
    "changelog_url": null,
    "date_published": "2022-01-12T20:41:27+00:00",
    "downloads": 22,
    "version_type": "release",
    "files": [
      {
        "hashes": {
          "sha1": "0fe87efacfd25c4c5e011cd1433e9be494b23b1c",
          "sha512": "d43e148d35d0267b49ed14a7bb4bb5879aa3ebbf5805c2fe125f0f0f19fd84994242bdcd568399c9ff80ecfbe0e975ab632d8c71773bc09d54cfc857f9a6f716"
        },
        "url": "https://cdn.modrinth.com/data/WCJmvhgU/versions/2022.1.12/waffles_Modpack-2022.1.12no-hydrogen.mrpack",
        "filename": "waffles_Modpack-2022.1.12no-hydrogen.mrpack",
        "primary": false,
        "size": 0
      }
    ],
    "dependencies": [],
    "game_versions": [
      "1.18.1"
    ],
    "loaders": [
      "fabric"
    ]
  }
 */

bool parseFile(QJsonObject & fileObj, Modrinth::Download & out) {
    out.primary = Json::requireBoolean(fileObj, "primary");
    out.size = Json::requireInteger(fileObj, "size");
    out.url = Json::requireString(fileObj, "url");
    out.filename = Json::requireString(fileObj, "filename");

    auto hashesObj = fileObj["hashes"].toObject();
    out.sha1 = Json::requireString(hashesObj, "sha1");

    if(!out.filename.endsWith(".mrpack")) {
        out.valid = false;
        return false;
    }
    else {
        out.valid = true;
        return true;
    }
}

namespace {

bool parseVersionsInto(QByteArray & input, Modrinth::Modpack& output) {
    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(input, &parse_error);
    if(parse_error.error != QJsonParseError::NoError)
    {
        qWarning() << "Error while parsing pack versions response from Modrinth at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << input;
        return false;
    }

    qDebug() << input;

    try
    {
        QVector<Modrinth::Version> newList;
        QJsonArray versions = Json::requireArray(doc);
        for (auto obj : versions)
        {
            auto packObj = obj.toObject();
            Modrinth::Version version;
            try
            {
                if (Json::ensureString(packObj, "client_side", "required") == QStringLiteral("unsupported"))
                    continue;
                version.name = Json::requireString(packObj, "version_number");
                version.released = Json::requireDateTime(packObj, "date_published");
                version.featured = Json::requireBoolean(packObj, "featured");
                auto versionTypeString = Json::requireString(packObj, "version_type");
                if(versionTypeString == "alpha") {
                    version.type = Modrinth::VersionType::Alpha;
                }
                else if(versionTypeString == "beta") {
                    version.type = Modrinth::VersionType::Beta;
                }
                else if (versionTypeString == "release") {
                    version.type = Modrinth::VersionType::Release;
                }
                else {
                    qWarning() << "Unknown version type of Modrinth modpack: " << versionTypeString;
                    version.type = Modrinth::VersionType::Unknown;
                }
                Modrinth::Download fallbackOut = {};
                auto filesArray = Json::requireArray(packObj, "files");
                for(int i = 0; i < filesArray.size(); i++) {
                    Modrinth::Download maybeFileOut = {};
                    QJsonObject fileObj = filesArray[i].toObject();
                    parseFile(fileObj, maybeFileOut);
                    if(i == 0) {
                        fallbackOut = maybeFileOut;
                    }
                    if(maybeFileOut.valid && maybeFileOut.primary) {
                        version.download = maybeFileOut;
                        break;
                    }
                }

                if(!version.download.valid) {
                    version.download = fallbackOut;
                }

                if(version.download.valid) {
                    newList.append(version);
                }
            }
            catch(const JSONValidationError &e)
            {
                qWarning() << "Error while loading pack from Modrinth: " << e.cause();
                continue;
            }
        }
        output.versions = newList;
        return true;
    }
    catch(const JSONValidationError &e)
    {
        qWarning() << "Error while parsing response from Modrinth: " << e.cause();
        return false;
    }
}
}

void Modrinth::ListModel::versionsRequestFinished()
{
    auto index = getIndexFromId(currentPackDetailRequest);
    if(index) {
        auto & modpack = modpacks[*index];
        parseVersionsInto(versionsResponse, modpack);
        modpack.versionsLoaded = LoadState::Loaded;
        emit packDataChanged(currentPackDetailRequest);
    }
    versionsPtr.reset();
    checkDetailsDone();
}

void Modrinth::ListModel::versionsRequestFailed()
{
    auto index = getIndexFromId(currentPackDetailRequest);
    if(index) {
        auto & modpack = modpacks[*index];
        if(modpack.versionsLoaded == LoadState::NotLoaded) {
            modpack.versionsLoaded = LoadState::Errored;
            emit packDataChanged(currentPackDetailRequest);
        }
    }
    versionsPtr.reset();
    checkDetailsDone();
}

void Modrinth::ListModel::checkDetailsDone()
{
    if(isPackDetailInProgress()) {
        return;
    }

    // all detail requests are finished
    currentPackDetailRequest.clear();

    // is there a new one queued?
    if(!queuedPackDetailRequest.isNull()) {
        getPackDetails(queuedPackDetailRequest);
        queuedPackDetailRequest.clear();
    }
}
