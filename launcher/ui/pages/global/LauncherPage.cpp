/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "LauncherPage.h"
#include "ui_LauncherPage.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

#include "updater/v1/UpdateChecker.h"

#include "settings/SettingsObject.h"
#include <FileSystem.h>
#include "Application.h"
#include "BuildConfig.h"

#include <QApplication>
#include <QProcess>
#include <DesktopServices.h>

// FIXME: possibly move elsewhere
enum InstSortMode
{
    // Sort alphabetically by name.
    Sort_Name,
    // Sort by which instance was launched most recently.
    Sort_LastLaunch
};

LauncherPage::LauncherPage(QWidget *parent) : QWidget(parent), ui(new Ui::LauncherPage)
{
    ui->setupUi(this);

    ui->sortingModeGroup->setId(ui->sortByNameBtn, Sort_Name);
    ui->sortingModeGroup->setId(ui->sortLastLaunchedBtn, Sort_LastLaunch);

    m_languageModel = APPLICATION->translations();
    loadSettings();

    // Updater; updater v2 has a page of its own (UpdatesPage)
    if(!BuildConfig.UPDATER_ENABLED || BuildConfig.UPDATER_VERSION == 2)
    {
        ui->updateSettingsBox->setHidden(true);
    }

    //move mac data button
    QFile file(QDir::current().absolutePath() + "/dontmovemacdata");
    if (!file.exists())
    {
        ui->migrateDataFolderMacBtn->setVisible(false);
    }
}

LauncherPage::~LauncherPage()
{
    delete ui;
}

bool LauncherPage::apply()
{
    applySettings();
    return true;
}

void LauncherPage::on_instDirBrowseBtn_clicked()
{
    QString raw_dir = QFileDialog::getExistingDirectory(this, tr("Instance Folder"), ui->instDirTextBox->text());

    // do not allow current dir - it's dirty. Do not allow dirs that don't exist
    if (!raw_dir.isEmpty() && QDir(raw_dir).exists())
    {
        QString cooked_dir = FS::NormalizePath(raw_dir);
        if (FS::checkProblemticPathJava(QDir(cooked_dir)))
        {
            QMessageBox warning;
            warning.setText(tr("You're trying to specify an instance folder which\'s path "
                               "contains at least one \'!\'. "
                               "Java is known to cause problems if that is the case, your "
                               "instances (probably) won't start!"));
            warning.setInformativeText(
                tr("Do you really want to use this path? "
                   "Selecting \"No\" will close this and not alter your instance path."));
            warning.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            int result = warning.exec();
            if (result == QMessageBox::Yes)
            {
                ui->instDirTextBox->setText(cooked_dir);
            }
        }
        else
        {
            ui->instDirTextBox->setText(cooked_dir);
        }
    }
}
void LauncherPage::on_instDirOpenBtn_clicked()
{
    DesktopServices::openDirectory(ui->instDirTextBox->text());
}


void LauncherPage::on_iconsDirBrowseBtn_clicked()
{
    QString raw_dir = QFileDialog::getExistingDirectory(this, tr("Icons Folder"), ui->iconsDirTextBox->text());

    // do not allow current dir - it's dirty. Do not allow dirs that don't exist
    if (!raw_dir.isEmpty() && QDir(raw_dir).exists())
    {
        QString cooked_dir = FS::NormalizePath(raw_dir);
        ui->iconsDirTextBox->setText(cooked_dir);
    }
}
void LauncherPage::on_iconsDirOpenBtn_clicked()
{
    DesktopServices::openDirectory(ui->iconsDirTextBox->text());
}

void LauncherPage::on_modsDirBrowseBtn_clicked()
{
    QString raw_dir = QFileDialog::getExistingDirectory(this, tr("Mods Folder"), ui->modsDirTextBox->text());

    // do not allow current dir - it's dirty. Do not allow dirs that don't exist
    if (!raw_dir.isEmpty() && QDir(raw_dir).exists())
    {
        QString cooked_dir = FS::NormalizePath(raw_dir);
        ui->modsDirTextBox->setText(cooked_dir);
    }
}
void LauncherPage::on_modsDirOpenBtn_clicked()
{
    DesktopServices::openDirectory(ui->modsDirTextBox->text());
}

void LauncherPage::on_skinsDirBrowseBtn_clicked()
{
    QString raw_dir = QFileDialog::getExistingDirectory(this, tr("Skins Folder"), ui->skinsDirTextBox->text());

    // do not allow current dir - it's dirty. Do not allow dirs that don't exist
    if (!raw_dir.isEmpty() && QDir(raw_dir).exists())
    {
        QString cooked_dir = FS::NormalizePath(raw_dir);
        ui->skinsDirTextBox->setText(cooked_dir);
    }
}
void LauncherPage::on_skinsDirOpenBtn_clicked()
{
    DesktopServices::openDirectory(ui->skinsDirTextBox->text());
}

void LauncherPage::on_migrateDataFolderMacBtn_clicked()
{
    QFile file(QDir::current().absolutePath() + "/dontmovemacdata");
    file.remove();
    QProcess::startDetached(qApp->arguments()[0]);
    qApp->quit();
}

void LauncherPage::applySettings()
{
    auto s = APPLICATION->settings();

    if (ui->resetNotificationsBtn->isChecked())
    {
        s->set("ShownNotifications", QString());
        s->set("ConfirmLogClear", true);
    }

    // Updates (updater v2's are on UpdatesPage)
    if(BuildConfig.UPDATER_VERSION != 2)
    {
        s->set("AutoUpdate", ui->autoUpdateCheckBox->isChecked());
    }
    // Console settings
    s->set("ShowConsole", ui->showConsoleCheck->isChecked());
    s->set("AutoCloseConsole", ui->autoCloseConsoleCheck->isChecked());
    s->set("ShowConsoleOnError", ui->showConsoleErrorCheck->isChecked());
    s->set("ConsoleMaxLines", ui->lineLimitSpinBox->value());
    s->set("ConsoleOverflowStop", ui->checkStopLogging->checkState() != Qt::Unchecked);

    // Folders
    // TODO: Offer to move instances to new instance folder.
    s->set("InstanceDir", ui->instDirTextBox->text());
    s->set("CentralModsDir", ui->modsDirTextBox->text());
    s->set("IconsDir", ui->iconsDirTextBox->text());
    s->set("SkinsDir", ui->skinsDirTextBox->text());

    auto sortMode = (InstSortMode)ui->sortingModeGroup->checkedId();
    switch (sortMode)
    {
    case Sort_LastLaunch:
        s->set("InstSortMode", "LastLaunch");
        break;
    case Sort_Name:
    default:
        s->set("InstSortMode", "Name");
        break;
    }
}
void LauncherPage::loadSettings()
{
    auto s = APPLICATION->settings();
    // Updates
    ui->autoUpdateCheckBox->setChecked(s->get("AutoUpdate").toBool());
    // Console settings
    ui->showConsoleCheck->setChecked(s->get("ShowConsole").toBool());
    ui->autoCloseConsoleCheck->setChecked(s->get("AutoCloseConsole").toBool());
    ui->showConsoleErrorCheck->setChecked(s->get("ShowConsoleOnError").toBool());
    ui->lineLimitSpinBox->setValue(s->get("ConsoleMaxLines").toInt());
    ui->checkStopLogging->setChecked(s->get("ConsoleOverflowStop").toBool());

    // Folders
    ui->instDirTextBox->setText(s->get("InstanceDir").toString());
    ui->modsDirTextBox->setText(s->get("CentralModsDir").toString());
    ui->iconsDirTextBox->setText(s->get("IconsDir").toString());
    ui->skinsDirTextBox->setText(s->get("SkinsDir").toString());

    QString sortMode = s->get("InstSortMode").toString();

    if (sortMode == "LastLaunch")
    {
        ui->sortLastLaunchedBtn->setChecked(true);
    }
    else
    {
        ui->sortByNameBtn->setChecked(true);
    }
}
