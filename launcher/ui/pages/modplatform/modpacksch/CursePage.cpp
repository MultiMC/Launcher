/*
 * Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
 * Copyright 2021 Philip T <me@phit.link>
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

#include "CursePage.h"
#include "ui_CursePage.h"

#include <QKeyEvent>

#include "ui/dialogs/NewInstanceDialog.h"
#include "modplatform/modpacksch/FTBPackInstallTask.h"

#include "HoeDown.h"

CursePage::CursePage(NewInstanceDialog* dialog, QWidget *parent)
        : QWidget(parent), ui(new Ui::CursePage), dialog(dialog)
{
    ui->setupUi(this);
    connect(ui->searchButton, &QPushButton::clicked, this, &CursePage::triggerSearch);

    filterModel = new ModpacksCH::FilterModel(this);
    listModel = new ModpacksCH::ListModel(this);
    filterModel->setSourceModel(listModel);
    ui->packView->setModel(filterModel);
    ui->packView->setSortingEnabled(true);
    ui->packView->header()->hide();
    ui->packView->setIndentation(0);

    ui->searchEdit->installEventFilter(this);

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    for(int i = 0; i < filterModel->getAvailableSortings().size(); i++)
    {
        ui->sortByBox->addItem(filterModel->getAvailableSortings().keys().at(i));
    }
    ui->sortByBox->setCurrentText(filterModel->translateCurrentSorting());

    connect(ui->sortByBox, &QComboBox::currentTextChanged, this, &CursePage::onSortingSelectionChanged);
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &CursePage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &CursePage::onVersionSelectionChanged);
}

CursePage::~CursePage()
{
    delete ui;
}

bool CursePage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->searchEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            triggerSearch();
            keyEvent->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool CursePage::shouldDisplay() const
{
    return true;
}

void CursePage::openedImpl()
{
    if(!initialised)
    {
        initialised = true;
    }

    suggestCurrent();
}

void CursePage::suggestCurrent()
{
    if(!isOpened)
    {
        return;
    }

    if (selectedVersion.isEmpty())
    {
        dialog->setSuggestedPack();
        return;
    }

    auto & selectedPack = selected;
    dialog->setSuggestedPack(selectedVersion, new ModpacksCH::PackInstallTask(selectedPack, selectedVersion, selected.packType));
    for(auto art : selectedPack.art) {
        if(art.type == "square") {
            QString editedLogoName;
            editedLogoName = selectedPack.name;

            listModel->getLogo(selectedPack.name, art.url, [this, editedLogoName](QString logo)
            {
                dialog->setSuggestedIconFromFile(logo + ".small", editedLogoName);
            });
        }
    }
}

void CursePage::triggerSearch()
{
    listModel->requestCurse(ui->searchEdit->text());
}

void CursePage::onSortingSelectionChanged(QString data)
{
    auto toSet = filterModel->getAvailableSortings().value(data);
    filterModel->setSorting(toSet);
}

void CursePage::onSelectionChanged(QModelIndex first, QModelIndex second)
{
    ui->versionSelectionBox->clear();

    if(!first.isValid())
    {
        if(isOpened)
        {
            dialog->setSuggestedPack();
        }
        return;
    }

    selected = filterModel->data(first, Qt::UserRole).value<ModpacksCH::Modpack>();
    auto & selectedPack = selected;

    HoeDown hoedown;
    QString output = hoedown.process(selectedPack.description.toUtf8());
    ui->packDescription->setHtml(output);

    // reverse foreach, so that the newest versions are first
    for (auto i = selectedPack.versions.size(); i--;) {
        ui->versionSelectionBox->addItem(selectedPack.versions.at(i).name);
    }

    suggestCurrent();
}

void CursePage::onVersionSelectionChanged(QString data)
{
    if(data.isNull() || data.isEmpty())
    {
        selectedVersion = "";
        return;
    }

    selectedVersion = data;
    suggestCurrent();
}
