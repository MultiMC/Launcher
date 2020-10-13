/*
 * Copyright 2023 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QDialog>
#include "net/NetJob.h"
#include "JREClient.h"

namespace Ui
{
class JREClientDialog;
}


class JREClientDialog : public QDialog
{
    Q_OBJECT

public:
    explicit JREClientDialog(QWidget *parent = 0);
    ~JREClientDialog();

protected:
    void closeEvent(QCloseEvent * ) override;

private:
    Ui::JREClientDialog *ui;
    JREClient::Ptr client;
};
