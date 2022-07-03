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

    updateDialogState();
}

CreateShortcutDialog::~CreateShortcutDialog()
{
    delete ui;
}

void CreateShortcutDialog::on_shortcutPathBrowse_clicked()
{
    QString linkExtension;
#ifdef Q_OS_LINUX
    linkExtension = "desktop";
#endif
#ifdef Q_OS_WIN
    linkExtension = "lnk";
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
    // Linux implementation using .desktop file
#ifdef Q_OS_LINUX
    // save the launcher icon to a file so we can use it in the shortcut
    if (!QFileInfo::exists(QCoreApplication::applicationDirPath() + "/shortcut-icon.png"))
    {
        QPixmap iconPixmap = QIcon(":/logo.svg").pixmap(64, 64);
        iconPixmap.save(QCoreApplication::applicationDirPath() + "/shortcut-icon.png");
    }

    QFile desktopFile(ui->shortcutPath->text());
    if (desktopFile.open(QIODevice::WriteOnly))
    {
        QTextStream stream(&desktopFile);
        qDebug() << m_instance->iconKey();
        stream << "[Desktop Entry]" << endl
                    << "Type=Application" << endl
                    << "Name=" << m_instance->name() << " - " << BuildConfig.LAUNCHER_DISPLAYNAME << endl
                    << "Exec=" << getLaunchCommand() << endl
                    << "Icon=" << QCoreApplication::applicationDirPath() << "/shortcut-icon.png" << endl;
        desktopFile.setPermissions(QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther
                                                | QFile::WriteOwner | QFile::ExeOwner | QFile::ExeGroup);
        desktopFile.close();
    }
#endif
    // TODO: implementations for other operating systems
}