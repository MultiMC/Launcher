/*
 * Copyright 2023 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "JREClientDialog.h"
#include "ui_JREClientDialog.h"
#include <QDebug>
#include "Application.h"
#include <settings/SettingsObject.h>
#include <Json.h>

#include "BuildConfig.h"

JREClientDialog::JREClientDialog(QWidget *parent) : QDialog(parent), ui(new Ui::JREClientDialog)
{
    ui->setupUi(this);
    client.reset(new JREClient(this));
    restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get("JREClientDialogGeometry").toByteArray()));
}

JREClientDialog::~JREClientDialog()
{
}

void JREClientDialog::closeEvent(QCloseEvent* evt)
{
    APPLICATION->settings()->set("JREClientDialogGeometry", saveGeometry().toBase64());
    QDialog::closeEvent(evt);
}

