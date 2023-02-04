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
    class SelectInstanceExportFormatDialog;
}
QT_END_NAMESPACE

class SelectInstanceExportFormatDialog : public QDialog
{
Q_OBJECT

public:
    explicit SelectInstanceExportFormatDialog(InstancePtr instance, QWidget *parent = nullptr);

    ~SelectInstanceExportFormatDialog() override;

private slots:
    void accept() override;

private:
    Ui::SelectInstanceExportFormatDialog *ui;
    InstancePtr m_instance;
};