/* Copyright 2026 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */
#include "IconPickerModel.h"

#include "icons/IconList.h"
#include "ui/instanceview/InstanceView.h"

IconPickerModel::IconPickerModel(IconList *icons, RenameFunction rename, QObject *parent)
    : QIdentityProxyModel(parent), m_icons(icons), m_rename(rename)
{
    setSourceModel(icons);
    // another icon became the default (set, renamed or its file gone): the check mark moves
    connect(icons, &IconList::iconUpdated, this, [this](QString key) {
        if (key == "default" && rowCount() > 0)
            emit dataChanged(index(0, 0), index(rowCount() - 1, 0));
    });
}

QVariant IconPickerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    switch (role)
    {
    case InstanceViewRoles::BadgesRole:
        if (index.data(Qt::UserRole).toString() == m_icons->defaultIconKey())
            return QStringList{"status-good"};
        return QVariant();
    case Qt::EditRole:
        // the name, as shown
        return QIdentityProxyModel::data(index, Qt::DisplayRole);
    default:
        return QIdentityProxyModel::data(index, role);
    }
}

Qt::ItemFlags IconPickerModel::flags(const QModelIndex &index) const
{
    auto flags = QIdentityProxyModel::flags(index);
    if (!index.isValid())
        return flags;
    // only icons from the icons folder have names to change: their files'
    auto icon = m_icons->icon(index.data(Qt::UserRole).toString());
    if (icon && icon->has(IconType::FileBased))
        flags |= Qt::ItemIsEditable;
    return flags;
}

bool IconPickerModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || !index.isValid())
        return false;
    return m_rename(index.data(Qt::UserRole).toString(), value.toString());
}
