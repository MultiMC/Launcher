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
        searchUrl = QString("https://staging-api.modrinth.com/v2/search?facets=[[%22project_type:modpack%22]]&index=%1&limit=25&offset=%2").arg(currentSort).arg(nextSearchOffset);
    }
    else
    {
        searchUrl = QString(
                "https://staging-api.modrinth.com/v2/search?facets=[[%22project_type:modpack%22]]&index=%1&limit=25&offset=%2&query=%3"
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

    QList<Modrinth::Modpack> newList;
    QJsonArray hits;
    int total_hits;

    try
    {
        auto obj = Json::requireObject(doc);
        hits = Json::requireArray(obj, "hits");
        total_hits = Json::requireInteger(obj, "total_hits");
    }
    catch(const JSONValidationError &e)
    {
        qWarning() << "Error while parsing response from Modrinth: " << e.cause();
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
    modpacks.append(newList);
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

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry("FlamePacks", QString("logos/%1").arg(logo.section(".", 0, 0)));
    auto *job = new NetJob(QString("Flame Icon Download %1").arg(logo), APPLICATION->network());
    job->addNetAction(Net::Download::makeCached(url, entry));

    auto fullPath = entry->getFullPath();
    QObject::connect(job, &NetJob::succeeded, this, [this, logo, fullPath]
    {
        QIcon icon(fullPath);
        QSize size = icon.actualSize(QSize(48, 48));
        if (size.width() < 48 && size.height() < 48)
        {
            /*while (size.width() < 48 && size.height() < 48)
                size *= 2;
            icon = icon.pixmap(48, 48).scaled(size);*/
            icon = icon.pixmap(48,48).scaled(48,48, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
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
