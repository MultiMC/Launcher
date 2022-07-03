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
#include "minecraft/ParseUtils.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// Dates when the game was affected by bugs that crashed the game when using the option to join a server on startup
const QDateTime MC_145102_START = timeFromS3Time("2019-02-20T14:56:58+00:00");
const QDateTime MC_145102_END = timeFromS3Time("2020-05-14T08:16:26+00:00");
const QDateTime MC_228828_START = timeFromS3Time("2021-03-10T15:24:38+00:00");
const QDateTime MC_228828_END = timeFromS3Time("2021-06-18T12:24:40+00:00");

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
    QString getLaunchArgs();

    void createShortcut();

#ifdef Q_OS_WIN
    void createWindowsLink(LPCSTR target, LPCSTR workingDir, LPCSTR args, LPCSTR filename, LPCSTR desc, LPCSTR iconPath);
#endif
};