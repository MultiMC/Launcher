/*
 * Copyright 2023 arthomnix
 *
 * This source is subject to the Microsoft Public License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include <QFileDialog>
#include <QStandardPaths>
#include <QProgressDialog>
#include <QMessageBox>
#include "ModrinthExportDialog.h"
#include "ui_ModrinthExportDialog.h"
#include "BaseInstance.h"
#include "modplatform/modrinth/ModrinthInstanceExportTask.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "ProgressDialog.h"
#include "CustomMessageBox.h"


ModrinthExportDialog::ModrinthExportDialog(InstancePtr instance, QWidget *parent) :
        QDialog(parent), ui(new Ui::ModrinthExportDialog), m_instance(instance)
{
    ui->setupUi(this);
    ui->name->setText(m_instance->name());
    ui->version->setText("1.0");
}

void ModrinthExportDialog::updateDialogState()
{
    ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(
            !ui->name->text().isEmpty()
            && !ui->version->text().isEmpty()
            && ui->file->text().endsWith(".mrpack")
            && (
                    !ui->includeDatapacks->isChecked()
                    || (!ui->datapacksPath->text().isEmpty() && QDir(m_instance->gameRoot() + "/" + ui->datapacksPath->text()).exists())
            )
    );
}

void ModrinthExportDialog::on_fileBrowseButton_clicked()
{
    QFileDialog dialog(this, tr("Select modpack file"), QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    dialog.setDefaultSuffix("mrpack");
    dialog.setNameFilter("Modrinth modpacks (*.mrpack)");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.selectFile(ui->name->text() + ".mrpack");

    if (dialog.exec()) {
        ui->file->setText(dialog.selectedFiles().at(0));
    }

    updateDialogState();
}

void ModrinthExportDialog::on_datapackPathBrowse_clicked()
{
    QFileDialog dialog(this, tr("Select global datapacks folder"), m_instance->gameRoot());
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::DirectoryOnly);

    if (dialog.exec()) {
        ui->datapacksPath->setText(QDir(m_instance->gameRoot()).relativeFilePath(dialog.selectedFiles().at(0)));
    }

    updateDialogState();
}

void ModrinthExportDialog::accept()
{
    Modrinth::ExportSettings settings;

    settings.name = ui->name->text();
    settings.version = ui->version->text();
    settings.description = ui->description->text();

    settings.includeGameConfig = ui->includeGameConfig->isChecked();
    settings.includeModConfigs = ui->includeModConfigs->isChecked();
    settings.includeResourcePacks = ui->includeResourcePacks->isChecked();
    settings.includeShaderPacks = ui->includeShaderPacks->isChecked();
    settings.treatDisabledAsOptional = ui->treatDisabledAsOptional->isChecked();

    if (ui->includeDatapacks->isChecked()) {
        settings.datapacksPath = ui->datapacksPath->text();
    }

    MinecraftInstancePtr minecraftInstance = std::dynamic_pointer_cast<MinecraftInstance>(m_instance);
    minecraftInstance->getPackProfile()->reload(Net::Mode::Offline);

    for (int i = 0; i < minecraftInstance->getPackProfile()->rowCount(); i++) {
        auto component = minecraftInstance->getPackProfile()->getComponent(i);
        if (component->isCustom()) {
            CustomMessageBox::selectable(
                    this,
                    tr("Warning"),
                    tr("Instance contains a custom component: %1\nThis cannot be exported to a Modrinth pack; the exported pack may not work correctly!")
                        .arg(component->getName()),
                    QMessageBox::Warning
            )->exec();
        }
    }

    settings.gameVersion = minecraftInstance->getPackProfile()->getComponentVersion("net.minecraft");
    settings.forgeVersion = minecraftInstance->getPackProfile()->getComponentVersion("net.minecraftforge");
    settings.fabricVersion = minecraftInstance->getPackProfile()->getComponentVersion("net.fabricmc.fabric-loader");
    settings.quiltVersion = minecraftInstance->getPackProfile()->getComponentVersion("org.quiltmc.quilt-loader");

    settings.exportPath = ui->file->text();

    auto *task = new Modrinth::InstanceExportTask(m_instance, settings);

    connect(task, &Task::failed, [this](QString reason)
        {
            QString text;
            if (reason.length() > 1000) {
                text = reason.left(1000) + "...";
            } else {
                text = reason;
            }
            CustomMessageBox::selectable(parentWidget(), tr("Error"), text, QMessageBox::Critical)->show();
        });
    connect(task, &Task::succeeded, [this, task]()
        {
            QStringList warnings = task->warnings();
            if(warnings.count())
            {
                CustomMessageBox::selectable(parentWidget(), tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
            }
        });
    ProgressDialog loadDialog(this);
    loadDialog.setSkipButton(true, tr("Abort"));
    loadDialog.execWithTask(task);

    QDialog::accept();
}

ModrinthExportDialog::~ModrinthExportDialog()
{
    delete ui;
}
