#include "FtbModel.h"

#include "BuildConfig.h"
#include "Env.h"
#include "MultiMC.h"
#include "Json.h"

namespace Ftb {

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

    ModpacksCH::Modpack pack = modpacks.at(pos);
    if(role == Qt::DisplayRole)
    {
        return pack.name;
    }
    else if (role == Qt::ToolTipRole)
    {
        return pack.synopsis;
    }
    else if(role == Qt::DecorationRole)
    {
        if(m_logoMap.contains(pack.name))
        {
            return (m_logoMap.value(pack.name));
        }

        for(auto art : pack.art) {
            if(art.type == "square") {
                ((ListModel *)this)->requestLogo(pack.name, art.url);
            }
        }

        QIcon icon = MMC->getThemedIcon("screenshot-placeholder");
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

void ListModel::performSearch()
{
    auto *netJob = new NetJob("Ftb::Search");
    auto searchUrl = QString(BuildConfig.MODPACKSCH_API_BASE_URL + "public/modpack/search/25?term=%1")
            .arg(currentSearchTerm);
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();
    QObject::connect(netJob, &NetJob::succeeded, this, &ListModel::searchRequestFinished);
    QObject::connect(netJob, &NetJob::failed, this, &ListModel::searchRequestFailed);
}

void ListModel::getLogo(const QString &logo, const QString &logoUrl, LogoCallback callback)
{
    if(m_logoMap.contains(logo))
    {
        callback(ENV.metacache()->resolveEntry("ModpacksCHPacks", QString("logos/%1").arg(logo.section(".", 0, 0)))->getFullPath());
    }
    else
    {
        requestLogo(logo, logoUrl);
    }
}

void ListModel::searchWithTerm(const QString &term)
{
    if(currentSearchTerm == term) {
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

void ListModel::searchRequestFinished()
{
    jobPtr.reset();
    remainingPacks.clear();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from FTB at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    auto packs = doc.object().value("packs").toArray();
    for(auto pack : packs) {
        auto packId = pack.toInt();
        remainingPacks.append(packId);
    }

    if(!remainingPacks.isEmpty()) {
        currentPack = remainingPacks.at(0);
        requestPack();
    }
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
}

void ListModel::requestPack()
{
    auto *netJob = new NetJob("Ftb::Search");
    auto searchUrl = QString(BuildConfig.MODPACKSCH_API_BASE_URL + "public/modpack/%1")
            .arg(currentPack);
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &ListModel::packRequestFinished);
    QObject::connect(netJob, &NetJob::failed, this, &ListModel::packRequestFailed);
}

void ListModel::packRequestFinished()
{
    jobPtr.reset();
    remainingPacks.removeOne(currentPack);

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from FTB at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    auto obj = doc.object();

    ModpacksCH::Modpack pack;
    try
    {
        ModpacksCH::loadModpack(pack, obj);
    }
    catch (const JSONValidationError &e)
    {
        qWarning() << "Error while reading pack manifest from FTB: " << e.cause();
        return;
    }

    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size());
    modpacks.append(pack);
    endInsertRows();

    if(!remainingPacks.isEmpty()) {
        currentPack = remainingPacks.at(0);
        requestPack();
    }
}

void ListModel::packRequestFailed(QString reason)
{
    jobPtr.reset();
    remainingPacks.removeOne(currentPack);
}

void ListModel::logoLoaded(QString logo, QIcon out)
{
    m_loadingLogos.removeAll(logo);
    m_logoMap.insert(logo, out);
    for(int i = 0; i < modpacks.size(); i++) {
        if(modpacks[i].name == logo) {
            emit dataChanged(createIndex(i, 0), createIndex(i, 0), {Qt::DecorationRole});
        }
    }
}

void ListModel::logoFailed(QString logo)
{
    m_failedLogos.append(logo);
    m_loadingLogos.removeAll(logo);
}

void ListModel::requestLogo(QString logo, QString url)
{
    if(m_loadingLogos.contains(logo) || m_failedLogos.contains(logo))
    {
        return;
    }

    MetaEntryPtr entry = ENV.metacache()->resolveEntry("ModpacksCHPacks", QString("logos/%1").arg(logo.section(".", 0, 0)));
    NetJob *job = new NetJob(QString("FTB Icon Download %1").arg(logo));
    job->addNetAction(Net::Download::makeCached(QUrl(url), entry));

    auto fullPath = entry->getFullPath();
    QObject::connect(job, &NetJob::finished, this, [this, logo, fullPath]
    {
        emit logoLoaded(logo, QIcon(fullPath));
        if(waitingCallbacks.contains(logo))
        {
            waitingCallbacks.value(logo)(fullPath);
        }
    });

    QObject::connect(job, &NetJob::failed, this, [this, logo]
    {
        emit logoFailed(logo);
    });

    job->start();

    m_loadingLogos.append(logo);
}

}
