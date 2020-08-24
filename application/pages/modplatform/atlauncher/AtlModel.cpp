#include "AtlModel.h"

#include <BuildConfig.h>
#include <MultiMC.h>

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
        return pack.description;
    }
    else if(role == Qt::DecorationRole)
    {
        return MMC->getThemedIcon("screenshot-placeholder");
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
    auto *netJob = new NetJob("Atl::Request");
    auto url = QString(BuildConfig.ATL_DOWNLOAD_SERVER + "launcher/json/packsnew.json");
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

    QList<ATLauncher::IndexedPack> newList;

    auto packs = doc.array();
    for(auto packRaw : packs) {
        auto packObj = packRaw.toObject();

        ATLauncher::IndexedPack pack;
        ATLauncher::loadIndexedPack(pack, packObj);

        // ignore packs without a published version
        if(pack.versions.length() == 0) continue;
        // only display public packs (for now)
        if(pack.type != ATLauncher::PackType::Public) continue;

        newList.append(pack);
    }

    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + newList.size() - 1);
    modpacks.append(newList);
    endInsertRows();
}

void ListModel::requestFailed(QString reason)
{
    jobPtr.reset();
}

}
