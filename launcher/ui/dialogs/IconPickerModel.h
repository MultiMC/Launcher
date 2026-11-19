/* Copyright 2026 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */
#pragma once

#include <QIdentityProxyModel>
#include <functional>

class IconList;

/**
 * The icon list as the icon picker shows it: a check mark on the default icon (as a badge, see
 * InstanceViewRoles::BadgesRole), and the names of icons from the icons folder editable in
 * place, which renames them.
 */
class IconPickerModel : public QIdentityProxyModel
{
public:
    /// Renames an icon; false when it didn't (the owner says why)
    using RenameFunction = std::function<bool(const QString &key, const QString &newKey)>;

    IconPickerModel(IconList *icons, RenameFunction rename, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

private: /* data */
    IconList *m_icons = nullptr;
    RenameFunction m_rename;
};
