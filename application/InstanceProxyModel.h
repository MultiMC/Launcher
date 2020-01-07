#pragma once

#include "groupview/GroupedProxyModel.h"

/**
 * A proxy model that is responsible for sorting instances into groups
 */
class InstanceProxyModel : public GroupedProxyModel
{
public:
    enum class SortMode {
        ByName,
        ByLastPlayed
    };
public:
    explicit InstanceProxyModel(QObject *parent = 0);
    QVariant data(const QModelIndex & index, int role) const override;
    void setSortMode(SortMode mode) {
        currentSortMode = mode;
    }

protected:
    virtual bool subSortLessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    SortMode currentSortMode = SortMode::ByName;
};
