#include "InstanceProxyModel.h"
#include "MultiMC.h"
#include <BaseInstance.h>
#include <icons/IconList.h>

InstanceProxyModel::InstanceProxyModel(QObject *parent) : GroupedProxyModel(parent)
{
}

QVariant InstanceProxyModel::data(const QModelIndex & index, int role) const
{
    QVariant data = QSortFilterProxyModel::data(index, role);
    if(role == Qt::DecorationRole)
    {
        return QVariant(MMC->icons()->getIcon(data.toString()));
    }
    return data;
}

bool InstanceProxyModel::subSortLessThan(const QModelIndex &left, const QModelIndex &right) const
{
    BaseInstance *pdataLeft = static_cast<BaseInstance *>(left.internalPointer());
    BaseInstance *pdataRight = static_cast<BaseInstance *>(right.internalPointer());
    switch(currentSortMode) {
        case SortMode::ByLastPlayed:
            return pdataLeft->lastLaunch() > pdataRight->lastLaunch();
        default:
        case SortMode::ByName:
            return QString::localeAwareCompare(pdataLeft->name(), pdataRight->name()) < 0;
    }
}
