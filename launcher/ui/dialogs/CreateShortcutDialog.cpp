/*
 * Copyright 2022-2023 arthomnix
 *
 * This source is subject to the Microsoft Public License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include <QFileDialog>
#include <BuildConfig.h>
#include <QMessageBox>
#include "CreateShortcutDialog.h"
#include "ui_CreateShortcutDialog.h"
#include "Application.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/WorldList.h"
#include "minecraft/VersionFilterData.h"
#include "minecraft/PackProfile.h"
#include "icons/IconList.h"

#ifdef Q_OS_WIN
#include <shobjidl.h>
#include <shlguid.h>

static void createWindowsLink(QString target, QString workingDir, QString args, QString filename, QString desc, QString iconPath)
{
    HRESULT result;
    IShellLink *link;

    CoInitialize(nullptr);
    result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *) &link);
    if (SUCCEEDED(result))
    {
        IPersistFile *file;

        auto wTarget = target.toStdWString();
        auto wWorkingDir = workingDir.toStdWString();
        auto wArgs = args.toStdWString();
        auto wDesc = desc.toStdWString();
        auto wIconPath = iconPath.toStdWString();

        link->SetPath(wTarget.c_str());
        link->SetWorkingDirectory(wWorkingDir.c_str());
        link->SetArguments(wArgs.c_str());
        link->SetDescription(wDesc.c_str());
        link->SetIconLocation(wIconPath.c_str(), 0);

        result = link->QueryInterface(IID_IPersistFile, (LPVOID *) &file);

        if (SUCCEEDED(result))
        {
            auto wFilename = filename.toStdWString();
            wchar_t path[MAX_PATH];
            file->Save(path, TRUE);
            file->Release();
        }
        link->Release();
    }
    CoUninitialize();
}
#endif

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

    // TODO: check if version is affected by crashing when joining servers on launch, ideally in meta

    bool instanceSupportsQuickPlay = false;

    auto mcInstance = std::dynamic_pointer_cast<MinecraftInstance>(instance);
    if (mcInstance)
    {
        mcInstance->getPackProfile()->reload(Net::Mode::Online);

        if (mcInstance->getPackProfile()->getComponent("net.minecraft")->getReleaseDateTime() >= g_VersionFilterData.quickPlayBeginsDate)
        {
            instanceSupportsQuickPlay = true;
            mcInstance->worldList()->update();
            for (const auto &world : mcInstance->worldList()->allWorlds())
            {
                ui->joinSingleplayer->addItem(world.folderName());
            }
        }
    }

    if (!instanceSupportsQuickPlay)
    {
        ui->joinServerRadioButton->setChecked(true);
        ui->joinSingleplayerRadioButton->setVisible(false);
        ui->joinSingleplayer->setVisible(false);
    }

    // Macs don't have any concept of a desktop shortcut, so force-enable the option to generate a shell script instead
#if defined(Q_OS_UNIX) && !defined(Q_OS_LINUX)
    ui->createScriptCheckBox->setEnabled(false);
    ui->createScriptCheckBox->setChecked(true);
#endif


    connect(ui->joinWorldCheckBox, &QCheckBox::toggled, this, &CreateShortcutDialog::updateDialogState);
    connect(ui->joinServer, &QLineEdit::textChanged, this, &CreateShortcutDialog::updateDialogState);
    connect(ui->launchOfflineCheckBox, &QCheckBox::stateChanged, this, &CreateShortcutDialog::updateDialogState);
    connect(ui->offlineUsername, &QLineEdit::textChanged, this, &CreateShortcutDialog::updateDialogState);
    connect(ui->offlineUsernameCheckBox, &QCheckBox::stateChanged, this, &CreateShortcutDialog::updateDialogState);
    connect(ui->profileComboBox, &QComboBox::currentTextChanged, this, &CreateShortcutDialog::updateDialogState);
    connect(ui->shortcutPath, &QLineEdit::textChanged, this, &CreateShortcutDialog::updateDialogState);
    connect(ui->useProfileCheckBox, &QCheckBox::stateChanged, this, &CreateShortcutDialog::updateDialogState);
    connect(ui->joinSingleplayer, &QComboBox::currentTextChanged, this, &CreateShortcutDialog::updateDialogState);
    connect(ui->joinServerRadioButton, &QRadioButton::toggled, this, &CreateShortcutDialog::updateDialogState);
    connect(ui->joinSingleplayerRadioButton, &QRadioButton::toggled, this, &CreateShortcutDialog::updateDialogState);

    connect(ui->joinWorldCheckBox, &QCheckBox::toggled, ui->joinServerRadioButton, &QRadioButton::setEnabled);
    connect(ui->joinWorldCheckBox, &QCheckBox::toggled, ui->joinSingleplayerRadioButton, &QRadioButton::setEnabled);
    connect(ui->joinServerRadioButton, &QRadioButton::toggled, ui->joinServer, &QLineEdit::setEnabled);
    connect(ui->joinSingleplayerRadioButton, &QRadioButton::toggled, ui->joinSingleplayer, &QComboBox::setEnabled);
    connect(ui->useProfileCheckBox, &QCheckBox::toggled, ui->profileComboBox, &QComboBox::setEnabled);
    connect(ui->launchOfflineCheckBox, &QCheckBox::toggled, ui->offlineUsernameCheckBox, &QCheckBox::setEnabled);
    updateDialogState();
}

CreateShortcutDialog::~CreateShortcutDialog()
{
    delete ui;
}

void CreateShortcutDialog::on_shortcutPathBrowse_clicked()
{
    QString linkExtension;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    linkExtension = ui->createScriptCheckBox->isChecked() ? "sh" : "desktop";
#endif
#ifdef Q_OS_MAC
    linkExtension = "command";
#endif
#ifdef Q_OS_WIN
    linkExtension = ui->createScriptCheckBox->isChecked() ? "bat" : "lnk";
#endif
    QFileDialog fileDialog(this, tr("Select shortcut path"), QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    fileDialog.setDefaultSuffix(linkExtension);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setFileMode(QFileDialog::AnyFile);
    fileDialog.selectFile(m_instance->name() + " - " + BuildConfig.LAUNCHER_DISPLAYNAME + "." + linkExtension);
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
            && (
                    !ui->joinWorldCheckBox->isChecked()
                    || (ui->joinServerRadioButton->isChecked() && !ui->joinServer->text().isEmpty())
                    || (ui->joinSingleplayerRadioButton->isChecked() && !ui->joinSingleplayer->currentText().isEmpty())
                )
            && (!ui->offlineUsernameCheckBox->isChecked() || !ui->offlineUsername->text().isEmpty())
            && (!ui->useProfileCheckBox->isChecked() || !ui->profileComboBox->currentText().isEmpty())
    );
    ui->joinServer->setEnabled(ui->joinWorldCheckBox->isChecked() && ui->joinServerRadioButton->isChecked());
    ui->joinSingleplayer->setEnabled(ui->joinWorldCheckBox->isChecked() && ui->joinSingleplayerRadioButton->isChecked());
    ui->offlineUsername->setEnabled(ui->launchOfflineCheckBox->isChecked() && ui->offlineUsernameCheckBox->isChecked());
    if (!ui->launchOfflineCheckBox->isChecked())
    {
        ui->offlineUsernameCheckBox->setChecked(false);
    }
}

QString CreateShortcutDialog::getLaunchCommand(bool escapeQuotesTwice)
{
    return "\"" + QDir::toNativeSeparators(QCoreApplication::applicationFilePath()).replace('"', escapeQuotesTwice ? "\\\\\"" : "\\\"") + "\""
        + getLaunchArgs(escapeQuotesTwice);
}

QString CreateShortcutDialog::getLaunchArgs(bool escapeQuotesTwice)
{
    return " -d \"" + QDir::toNativeSeparators(QDir::currentPath()).replace('"', escapeQuotesTwice ? "\\\\\"" : "\\\"") + "\""
           + " -l \"" + m_instance->id() + "\""
           + (ui->joinServerRadioButton->isChecked() ? " -s \"" + ui->joinServer->text() + "\"" : "")
           + (ui->joinSingleplayerRadioButton->isChecked() ? " -w \"" + ui->joinSingleplayer->currentText() + "\"" : "")
           + (ui->useProfileCheckBox->isChecked() ? " -a \"" + ui->profileComboBox->currentText() + "\"" : "")
           + (ui->launchOfflineCheckBox->isChecked() ? " -o" : "")
           + (ui->offlineUsernameCheckBox->isChecked() ? " -n \"" + ui->offlineUsername->text() + "\"" : "");
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
            shortcutText = "#!/bin/sh\n"
                           // FIXME: is there a way to use the launcher script instead of the raw binary here?
                    "cd \"" + QDir::currentPath().replace('"', "\\\"") + "\"\n"
                    + getLaunchCommand() + " &\n";
        } else
            // freedesktop.org desktop entry
        {
            // save the launcher icon to a file so we can use it in the shortcut
            if (!QFileInfo::exists(QDir::currentPath() + "/icons/shortcut-icon.png"))
            {
                QPixmap iconPixmap = QIcon(":/logo.svg").pixmap(64, 64);
                iconPixmap.save(QDir::currentPath() + "/icons/shortcut-icon.png");
            }

            shortcutText = "[Desktop Entry]\n"
                           "Type=Application\n"
                           "Name=" + m_instance->name() + " - " + BuildConfig.LAUNCHER_DISPLAYNAME + "\n"
                           + "Exec=" + getLaunchCommand(true) + "\n"
                           + "Path=" + QDir::currentPath() + "\n"
                           + "Icon=" + QDir::currentPath() + "/icons/shortcut-icon.png\n";

        }
#endif
#ifdef Q_OS_WIN
        // Windows batch script implementation
        shortcutText = "@ECHO OFF\r\n"
                       "CD \"" + QDir::toNativeSeparators(QDir::currentPath()) + "\"\r\n"
                       "START /B \"\" " + getLaunchCommand() + "\r\n";
#endif
        QFile shortcutFile(ui->shortcutPath->text());
        if (shortcutFile.open(QIODevice::WriteOnly))
        {
            QTextStream stream(&shortcutFile);
            stream << shortcutText;
            shortcutFile.setPermissions(QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther | QFile::WriteOwner | QFile::ExeOwner | QFile::ExeGroup);
            shortcutFile.close();
        }
#ifdef Q_OS_WIN
    }
    else
    {
        if (!QFileInfo::exists(QDir::currentPath() + "/icons/shortcut-icon.ico"))
        {
            QPixmap iconPixmap = QIcon(":/logo.svg").pixmap(64, 64);
            iconPixmap.save(QDir::currentPath() + "/icons/shortcut-icon.ico");
        }
        
        createWindowsLink(
            QDir::toNativeSeparators(QCoreApplication::applicationFilePath()),
            QDir::toNativeSeparators(QDir::currentPath()),
            getLaunchArgs(),
            ui->shortcutPath->text(),
            (m_instance->name() + " - " + BuildConfig.LAUNCHER_DISPLAYNAME),
            QDir::toNativeSeparators(QDir::currentPath() + "/icons/shortcut-icon.ico")
        );
    }
#endif
}
