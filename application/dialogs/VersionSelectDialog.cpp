/* Copyright 2013-2019 MultiMC Contributors
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

#include "VersionSelectDialog.h"
#include "ui_VersionSelectDialog.h"

#include "dialogs/ProgressDialog.h"
#include "CustomMessageBox.h"

#include <BaseVersion.h>
#include <BaseVersionList.h>
#include <tasks/Task.h>
#include <QDebug>
#include "MultiMC.h"
#include <VersionProxyModel.h>
#include <widgets/VersionSelectWidget.h>
#include <QPushButton>

VersionSelectDialog::VersionSelectDialog(BaseVersionList *vlist, QString title, QWidget *parent, bool cancelable)
    : QDialog(parent), ui(new Ui::VersionSelectDialog), m_vlist(vlist)
{
    setObjectName(QStringLiteral("VersionSelectDialog"));

    ui->setupUi(this);

    setWindowModality(Qt::WindowModal);
    setWindowTitle(title);

    if (!cancelable)
    {
        ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
    }
}

VersionSelectDialog::~VersionSelectDialog()
{
    delete ui;
}

void VersionSelectDialog::setCurrentVersion(const QString& version)
{
    ui->versionSelect->setCurrentVersion(version);
}

void VersionSelectDialog::setEmptyString(QString emptyString)
{
    ui->versionSelect->setEmptyString(emptyString);
}

void VersionSelectDialog::setEmptyErrorString(QString emptyErrorString)
{
    ui->versionSelect->setEmptyErrorString(emptyErrorString);
}

void VersionSelectDialog::setResizeOn(int column)
{
    resizeOnColumn = column;
}

int VersionSelectDialog::exec()
{
    QDialog::open();
    ui->versionSelect->initialize(m_vlist);
    if(resizeOnColumn != -1)
    {
        ui->versionSelect->setResizeOn(resizeOnColumn);
    }
    return QDialog::exec();
}

BaseVersionPtr VersionSelectDialog::selectedVersion() const
{
    return ui->versionSelect->selectedVersion();
}

void VersionSelectDialog::setExactFilter(BaseVersionList::ModelRoles role, QString filter)
{
    ui->versionSelect->setExactFilter(role, filter);
}

void VersionSelectDialog::setFuzzyFilter(BaseVersionList::ModelRoles role, QString filter)
{
    ui->versionSelect->setFuzzyFilter(role, filter);
}

void VersionSelectDialog::setFilterBoxVisible(bool visible)
{
    ui->versionSelect->setFilterBoxVisible(visible);
}