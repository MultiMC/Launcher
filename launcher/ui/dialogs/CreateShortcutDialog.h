/*
 * Copyright 2022 arthomnix
 *
 * This source is subject to the Microsoft Public License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QDialog>
#include "minecraft/auth/MinecraftAccount.h"
#include "BaseInstance.h"

namespace Ui
{
    class CreateShortcutDialog;
}

class CreateShortcutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateShortcutDialog(QWidget *parent = nullptr, InstancePtr instance = nullptr);
    ~CreateShortcutDialog() override;

private
slots:
    void on_shortcutPathBrowse_clicked();
    void updateDialogState();
    void accept() override;

private:
    Ui::CreateShortcutDialog *ui;
    InstancePtr m_instance;

    QString getLaunchCommand();

    void createShortcut();
};