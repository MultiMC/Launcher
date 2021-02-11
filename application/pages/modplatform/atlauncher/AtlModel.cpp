#include "AtlModel.h"

#include <BuildConfig.h>
#include <MultiMC.h>
#include <Env.h>

namespace Atl {

ListModel::ListModel(QObject *parent) : QAbstractListModel(parent)
{
}

ListModel::~ListModel()
{
}

int ListModel::rowCount(const QModelIndex &parent) const
{
    return modpacks.size();
}

int ListModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant ListModel::data(const QModelIndex &index, int role) const
{
    int pos = index.row();
    if(pos >= modpacks.size() || pos < 0 || !index.isValid())
    {
        return QString("INVALID INDEX %1").arg(pos);
    }

    ATLauncher::IndexedPack pack = modpacks.at(pos);
    if(role == Qt::DisplayRole)
    {
        return pack.name;
    }
    else if (role == Qt::ToolTipRole)
    {
        return pack.name;
    }
    else if(role == Qt::DecorationRole)
    {
        if(m_logoMap.contains(pack.safeName))
        {
            return (m_logoMap.value(pack.safeName));
        }
        auto icon = MMC->getThemedIcon("atlauncher-placeholder");

        auto url = QString(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "launcher/images/%1.png").arg(pack.safeName.toLower());
        ((ListModel *)this)->requestLogo(pack.safeName, url);

        return icon;
    }
    else if(role == Qt::UserRole)
    {
        QVariant v;
        v.setValue(pack);
        return v;
    }

    return QVariant();
}

void ListModel::request()
{
    beginResetModel();
    modpacks.clear();
    cachedpacks.clear();
    remainingPacks.clear();
    endResetModel();

    auto *netJob = new NetJob("Atl::Request");
    auto url = QString(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "launcher/json/packsnew.json");
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(url), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &ListModel::requestFinished);
    QObject::connect(netJob, &NetJob::failed, this, &ListModel::requestFailed);
}

void ListModel::requestFinished()
{
    jobPtr.reset();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from ATL at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    auto packs = doc.array();
    for(auto packRaw : packs) {
        auto packObj = packRaw.toObject();

        ATLauncher::IndexedPack pack;
        ATLauncher::loadIndexedPack(pack, packObj);
        cachedpacks.append(pack);
        auto packId = pack.id;
        remainingPacks.append(packId);
    }

    requestPacks();
}

void ListModel::requestFailed(QString reason)
{
    jobPtr.reset();
}

void ListModel::getLogo(const QString &logo, const QString &logoUrl, LogoCallback callback)
{
    if(m_logoMap.contains(logo))
    {
        callback(ENV.metacache()->resolveEntry("ATLauncherPacks", QString("logos/%1").arg(logo.section(".", 0, 0)))->getFullPath());
    }
    else
    {
        requestLogo(logo, logoUrl);
    }
}

void ListModel::logoFailed(QString logo)
{
    m_failedLogos.append(logo);
    m_loadingLogos.removeAll(logo);
}

void ListModel::logoLoaded(QString logo, QIcon out)
{
    m_loadingLogos.removeAll(logo);
    m_logoMap.insert(logo, out);

    for(int i = 0; i < modpacks.size(); i++) {
        if(modpacks[i].safeName == logo) {
            emit dataChanged(createIndex(i, 0), createIndex(i, 0), {Qt::DecorationRole});
        }
    }
}

void ListModel::requestLogo(QString file, QString url)
{
    if(m_loadingLogos.contains(file) || m_failedLogos.contains(file))
    {
        return;
    }

    MetaEntryPtr entry = ENV.metacache()->resolveEntry("ATLauncherPacks", QString("logos/%1").arg(file.section(".", 0, 0)));
    NetJob *job = new NetJob(QString("ATLauncher Icon Download %1").arg(file));
    job->addNetAction(Net::Download::makeCached(QUrl(url), entry));

    auto fullPath = entry->getFullPath();
    QObject::connect(job, &NetJob::succeeded, this, [this, file, fullPath]
    {
        emit logoLoaded(file, QIcon(fullPath));
        if(waitingCallbacks.contains(file))
        {
            waitingCallbacks.value(file)(fullPath);
        }
    });

    QObject::connect(job, &NetJob::failed, this, [this, file]
    {
        emit logoFailed(file);
    });

    job->start();

    m_loadingLogos.append(file);
}

void ListModel::performSearch()
{
    beginResetModel();
    modpacks.clear();
    cachedpacks.clear();
    remainingPacks.clear();
    endResetModel();

    auto *netJob = new NetJob("Atl::Search");
    QString searchUrl;
    auto url = QString(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "launcher/json/packsnew.json");
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(url), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &ListModel::searchRequestFinished);
    QObject::connect(netJob, &NetJob::failed, this, &ListModel::searchRequestFailed);
}

void ListModel::searchRequestFinished()
{
    jobPtr.reset();
    remainingPacks.clear();
    cachedpacks.clear();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from ATL at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    QString sanitizedSearchTerm = currentSearchTerm.replace(QRegularExpression("[^A-Za-z0-9]"), "");
    auto packs = doc.array();
    for(auto packRaw : packs) {
        auto packObj = packRaw.toObject();

        ATLauncher::IndexedPack pack;
        ATLauncher::loadIndexedPack(pack, packObj);
        cachedpacks.append(pack);

        if(pack.safeName.contains(sanitizedSearchTerm, Qt::CaseInsensitive))
        {
            auto packId = pack.id;
            remainingPacks.append(packId);
        }
    }

    requestRemaining = !remainingPacks.isEmpty();
    if(requestRemaining) requestPacks();
    cachedpacks.clear();
}

void ListModel::searchRequestFailed(QString reason)
{
    jobPtr.reset();
    remainingPacks.clear();

    if(searchState == ResetRequested) {
        beginResetModel();
        modpacks.clear();
        endResetModel();

        performSearch();
    } else {
        searchState = Finished;
    }
    requestRemaining = false;
    cachedpacks.clear();
}

void ListModel::searchWithTerm(const QString &term)
{
    if(currentSearchTerm == term && currentSearchTerm.isNull() == term.isNull()) {
        return;
    }
    currentSearchTerm = term;
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
    performSearch();
}

void ListModel::requestPacks()
{
    for(auto pack : cachedpacks)
    {
        if(remainingPacks.isEmpty()) continue;

        auto packId = pack.id;
        bool validPack = false;

        for(auto currentPack : remainingPacks)
        {
            if(currentPack == packId)
            {
                remainingPacks.removeOne(currentPack);
                validPack = true;
            }
        }

        // ignore packs without a published version
        if(pack.versions.length() == 0) continue;
        // only display public packs (for now)
        if(pack.type != ATLauncher::PackType::Public) continue;
        // ignore "system" packs (Vanilla, Vanilla with Forge, etc)
        if(pack.system) continue;

        if(validPack) cachedpacks.append(pack);
    }

    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + cachedpacks.size() - 1);
    modpacks.append(cachedpacks);
    endInsertRows();
}

}
