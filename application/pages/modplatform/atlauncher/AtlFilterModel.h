#pragma once

#include <QtCore/QSortFilterProxyModel>

namespace Atl {

class FilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    FilterModel(QObject* parent = Q_NULLPTR);
    enum Sorting {
        ByPopularity,
        ByGameVersion,
        ByName,
    };
    const QMap<QString, Sorting> getAvailableSortings();
    QString translateCurrentSorting();
    void setSorting(Sorting sorting);
    Sorting getCurrentSorting();
    void setSearchTerm(QString term);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QMap<QString, Sorting> sortings;
    Sorting currentSorting;
    QString searchTerm;

};

}
