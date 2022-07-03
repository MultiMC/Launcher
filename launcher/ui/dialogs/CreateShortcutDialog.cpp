/*
 * Copyright 2022 arthomnix
 *
 * This source is subject to the Microsoft Public License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include <fstream>
#include <iostream>
#include <QFileDialog>
#include <BuildConfig.h>
#include "CreateShortcutDialog.h"
#include "ui_CreateShortcutDialog.h"
#include "Application.h"
#include "minecraft/auth/AccountList.h"
#include "icons/IconList.h"

CreateShortcutDialog::CreateShortcutDialog(QWidget *parent, InstancePtr instance)
    :QDialog(parent), ui(new Ui::CreateShortcutDialog), m_instance(instance)
{
    ui->setupUi(this);

    QStringList accountNameList;
    auto accounts = APPLICATION->accounts();

    for (int i = 0; i < accounts->count(); i++)
    {
        accountNameList.append(accounts->at(i)->profileName());
    }

    ui->profileComboBox->addItems(accountNameList);

    if (accounts->defaultAccount())
    {
        ui->profileComboBox->setCurrentText(accounts->defaultAccount()->profileName());
    }

#if defined(Q_OS_WIN) || (defined(Q_OS_UNIX) && !defined(Q_OS_LINUX))
    ui->createScriptCheckBox->setEnabled(false);
    ui->createScriptCheckBox->setChecked(true);
#endif

    updateDialogState();
}

CreateShortcutDialog::~CreateShortcutDialog()
{
    delete ui;
}

void CreateShortcutDialog::on_shortcutPathBrowse_clicked()
{
    QString linkExtension;
#ifdef Q_OS_UNIX
    linkExtension = ui->createScriptCheckBox->isChecked() ? "sh" : "desktop";
#endif
#ifdef Q_OS_WIN
    linkExtension = ui->createScriptCheckBox->isChecked() ? "bat" : "lnk";
#endif
    QFileDialog fileDialog(this, tr("Select shortcut path"), QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    fileDialog.setDefaultSuffix(linkExtension);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setFileMode(QFileDialog::AnyFile);
    fileDialog.selectFile(m_instance->id() + "." + linkExtension);
    if (fileDialog.exec())
    {
        ui->shortcutPath->setText(fileDialog.selectedFiles().at(0));
    }
    updateDialogState();
}

void CreateShortcutDialog::accept()
{
    createShortcut();
    QDialog::accept();
}


void CreateShortcutDialog::updateDialogState()
{

    ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(
            !ui->shortcutPath->text().isEmpty()
            && (!ui->joinServerCheckBox->isChecked() || !ui->joinServer->text().isEmpty())
            && (!ui->offlineUsernameCheckBox->isChecked() || !ui->offlineUsername->text().isEmpty())
            && (!ui->useProfileCheckBox->isChecked() || !ui->profileComboBox->currentText().isEmpty())
    );
    ui->joinServer->setEnabled(ui->joinServerCheckBox->isChecked());
    ui->profileComboBox->setEnabled(ui->useProfileCheckBox->isChecked());
    ui->offlineUsernameCheckBox->setEnabled(ui->launchOfflineCheckBox->isChecked());
    ui->offlineUsername->setEnabled(ui->launchOfflineCheckBox->isChecked() && ui->offlineUsernameCheckBox->isChecked());
}

QString CreateShortcutDialog::getLaunchCommand()
{
    return QCoreApplication::applicationFilePath()
        + " -l " + m_instance->id()
        + (ui->joinServerCheckBox->isChecked() ? " -s " + ui->joinServer->text() : "")
        + (ui->useProfileCheckBox->isChecked() ? " -a " + ui->profileComboBox->currentText() : "")
        + (ui->launchOfflineCheckBox->isChecked() ? " -o" : "")
        + (ui->offlineUsernameCheckBox->isChecked() ? " -n " + ui->offlineUsername->text() : "");
}

void CreateShortcutDialog::createShortcut()
{
#ifdef Q_OS_WIN
    if (ui->createScriptCheckBox->isChecked()) // on windows, creating .lnk shortcuts requires specific win32 api stuff
                                               // rather than just writing a text file
    {
#endif
        QString shortcutText;
#ifdef Q_OS_UNIX
        // Unix shell script
        if (ui->createScriptCheckBox->isChecked())
        {
            shortcutText = "#!/bin/sh\n" + getLaunchCommand() + " &\n";
        } else
            // freedesktop.org desktop entry
        {
            // save the launcher icon to a file so we can use it in the shortcut
            if (!QFileInfo::exists(QCoreApplication::applicationDirPath() + "/shortcut-icon.png"))
            {
                QPixmap iconPixmap = QIcon(":/logo.svg").pixmap(64, 64);
                iconPixmap.save(QCoreApplication::applicationDirPath() + "/shortcut-icon.png");
            }

            shortcutText = "[Desktop Entry]\n"
                           "Type=Application\n"
                           "Name=" + m_instance->name() + " - " + BuildConfig.LAUNCHER_DISPLAYNAME + "\n"
                           + "Exec=" + getLaunchCommand() + "\n"
                           + "Icon=" + QCoreApplication::applicationDirPath() + "/shortcut-icon.png\n";

        }
#endif
#ifdef Q_OS_WIN
        // Windows batch script implementation
        if (ui->createScriptCheckBox->isChecked())
        {
            shortcutText = "@ECHO OFF\r\n"
                           "START /B " + getLaunchCommand() + "\r\n";
        }
        else
        {
            // TODO: windows .lnk implementation
        }
#endif
        QFile shortcutFile(ui->shortcutPath->text());
        if (shortcutFile.open(QIODevice::WriteOnly))
        {
            QTextStream stream(&shortcutFile);
            stream << shortcutText;
            shortcutFile.setPermissions(QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther
                                        | QFile::WriteOwner | QFile::ExeOwner | QFile::ExeGroup);
            shortcutFile.close();
        }
#ifdef Q_OS_WIN
    }
#endif
}