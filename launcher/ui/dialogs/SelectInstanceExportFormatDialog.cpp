/*
 * Copyright 2023 arthomnix
 *
 * This source is subject to the Microsoft Public License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "SelectInstanceExportFormatDialog.h"
#include "ui_SelectInstanceExportFormatDialog.h"
#include "BuildConfig.h"
#include "ModrinthExportDialog.h"


SelectInstanceExportFormatDialog::SelectInstanceExportFormatDialog(InstancePtr instance, QWidget *parent) :
        QDialog(parent), ui(new Ui::SelectInstanceExportFormatDialog), m_instance(instance)
{
    ui->setupUi(this);
    ui->mmcFormat->setText(BuildConfig.LAUNCHER_NAME);
}

void SelectInstanceExportFormatDialog::accept()
{
    if (ui->mmcFormat->isChecked()) {
        ExportInstanceDialog dlg(m_instance, parentWidget());
        QDialog::accept();
        dlg.exec();
    } else if (ui->modrinthFormat->isChecked()) {
        ModrinthExportDialog dlg(m_instance, parentWidget());
        QDialog::accept();
        dlg.exec();
    }
}

SelectInstanceExportFormatDialog::~SelectInstanceExportFormatDialog()
{
    delete ui;
}
