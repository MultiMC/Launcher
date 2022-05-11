/*
 * Copyright 2013-2021 MultiMC Contributors
 * Copyright 2021-2022 kb1000
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

#include "ModrinthModel.h"
#include "ModrinthPage.h"
#include "ui/dialogs/NewInstanceDialog.h"

#include "ui_ModrinthPage.h"

#include <QKeyEvent>

ModrinthPage::ModrinthPage(NewInstanceDialog *dialog, QWidget *parent) : QWidget(parent), ui(new Ui::ModrinthPage), dialog(dialog)
{
    ui->setupUi(this);
    connect(ui->searchButton, &QPushButton::clicked, this, &ModrinthPage::triggerSearch);
    ui->searchEdit->installEventFilter(this);
    model = new Modrinth::ListModel(this);
    ui->packView->setModel(model);

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    ui->sortByBox->addItem(tr("Sort by relevance"), QStringLiteral("relevance"));
    ui->sortByBox->addItem(tr("Sort by total downloads"), QStringLiteral("downloads"));
    ui->sortByBox->addItem(tr("Sort by follow count"), QStringLiteral("follows"));
    ui->sortByBox->addItem(tr("Sort by creation date"), QStringLiteral("newest"));
    ui->sortByBox->addItem(tr("Sort by last updated"), QStringLiteral("updated"));

    connect(ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ModrinthPage::onSelectionChanged);
    //connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &ModrinthPage::onVersionSelectionChanged);
}

ModrinthPage::~ModrinthPage()
{
    delete ui;
}

void ModrinthPage::openedImpl()
{
    BasePage::openedImpl();
    triggerSearch();
}

bool ModrinthPage::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->searchEdit && event->type() == QEvent::KeyPress) {
        auto *keyEvent = reinterpret_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            this->triggerSearch();
            keyEvent->accept();
            return true;
        }
    }
    return QObject::eventFilter(watched, event);
}

void ModrinthPage::triggerSearch() {
    model->searchWithTerm(ui->searchEdit->text(), ui->sortByBox->itemData(ui->sortByBox->currentIndex()).toString());
}

void ModrinthPage::onSelectionChanged(QModelIndex first, QModelIndex second) {
    if(!first.isValid())
    {
        if(isOpened)
        {
            dialog->setSuggestedPack();
        }
        //ui->frame->clear();
        return;
    }

    current = model->data(first, Qt::UserRole).value<Modrinth::Modpack>();
    suggestCurrent();
}

void ModrinthPage::suggestCurrent() {

}