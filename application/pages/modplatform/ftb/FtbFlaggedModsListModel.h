#pragma once

#include <QAbstractListModel>
#include "FtbPage.h"

class FlaggedModsListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Columns
    {
        EnabledColumn = 0,
        NameColumn,
        FlaggedForColumn,
        DescriptionColumn,
    };

    FlaggedModsListModel(QWidget *parent, QVector<ModpacksCH::FlaggedMod> flaggedMods);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

public slots:
    void enableDisableMods();

private:
    QVector<ModpacksCH::FlaggedMod> m_flaggedMods;
    QMap<QString, bool> m_modState;

};
