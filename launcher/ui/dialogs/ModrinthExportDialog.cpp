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
#include "ModrinthInstanceExportTask.h"
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
            && !ui->file->text().isEmpty()
    );
}

void ModrinthExportDialog::on_fileBrowseButton_clicked()
{
    QFileDialog dialog(this, tr("Select modpack file"), QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    dialog.setDefaultSuffix("mrpack");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.selectFile(ui->name->text() + ".mrpack");

    if (dialog.exec()) {
        ui->file->setText(dialog.selectedFiles().at(0));
    }

    updateDialogState();
}

void ModrinthExportDialog::accept()
{
    ModrinthExportSettings settings;

    settings.name = ui->name->text();
    settings.version = ui->version->text();
    settings.description = ui->description->text();

    settings.includeGameConfig = ui->includeGameConfig->isChecked();
    settings.includeModConfigs = ui->includeModConfigs->isChecked();
    settings.includeResourcePacks = ui->includeResourcePacks->isChecked();
    settings.includeShaderPacks = ui->includeShaderPacks->isChecked();

    MinecraftInstancePtr minecraftInstance = std::dynamic_pointer_cast<MinecraftInstance>(m_instance);
    minecraftInstance->getPackProfile()->reload(Net::Mode::Offline);

    auto minecraftComponent = minecraftInstance->getPackProfile()->getComponent("net.minecraft");
    auto forgeComponent = minecraftInstance->getPackProfile()->getComponent("net.minecraftforge");
    auto fabricComponent = minecraftInstance->getPackProfile()->getComponent("net.fabricmc.fabric-loader");
    auto quiltComponent = minecraftInstance->getPackProfile()->getComponent("org.quiltmc.quilt-loader");

    if (minecraftComponent) {
        settings.gameVersion = minecraftComponent->getVersion();
    }
    if (forgeComponent) {
        settings.forgeVersion = forgeComponent->getVersion();
    }
    if (fabricComponent) {
        settings.fabricVersion = fabricComponent->getVersion();
    }
    if (quiltComponent) {
        settings.quiltVersion = quiltComponent->getVersion();
    }

    settings.exportPath = ui->file->text();

    auto *task = new ModrinthInstanceExportTask(m_instance, settings);

    connect(task, &Task::failed, [this](QString reason)
        {
            CustomMessageBox::selectable(parentWidget(), tr("Error"), reason, QMessageBox::Critical)->show();
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
