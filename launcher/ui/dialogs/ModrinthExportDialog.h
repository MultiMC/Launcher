/*
 * Copyright 2023 arthomnix
 *
 * This source is subject to the Microsoft Public License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QDialog>
#include "ExportInstanceDialog.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class ModrinthExportDialog;
}
QT_END_NAMESPACE

class ModrinthExportDialog : public QDialog
{
Q_OBJECT

public:
    explicit ModrinthExportDialog(InstancePtr instance, QWidget *parent = nullptr);

    ~ModrinthExportDialog() override;

private slots:
    void on_fileBrowseButton_clicked();
    void on_datapackPathBrowse_clicked();
    void accept() override;
    void updateDialogState();

private:
    Ui::ModrinthExportDialog *ui;
    InstancePtr m_instance;
};