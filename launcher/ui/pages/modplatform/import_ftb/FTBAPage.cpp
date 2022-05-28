/*
 * Copyright 2022 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "FTBAPage.h"
#include "ui_FTBAPage.h"

#include "Application.h"

#include "ui/dialogs/NewInstanceDialog.h"

#include "PackInstallTask.h"
#include "Model.h"

namespace ImportFTB {

FTBAPage::FTBAPage(NewInstanceDialog* dialog, QWidget *parent)
    : QWidget(parent), dialog(dialog), ui(new Ui::FTBAPage)
{
    ui->setupUi(this);

    packModel = new Model(this);

    ui->packList->setModel(packModel);
    ui->packList->setSortingEnabled(true);
    ui->packList->header()->hide();
    ui->packList->setIndentation(0);
    ui->packList->setIconSize(QSize(42, 42));

    connect(ui->packList->selectionModel(), &QItemSelectionModel::currentChanged, this, &FTBAPage::onPublicPackSelectionChanged);

    ui->packList->selectionModel()->reset();

    currentList = ui->packList;
    currentModpackInfo = ui->packDescription;

    currentList->selectionModel()->reset();
    QModelIndex idx = currentList->currentIndex();
    if(idx.isValid())
    {
        auto pack = packModel->data(idx, Qt::UserRole).value<Modpack>();
        onPackSelectionChanged(&pack);
    }
    else
    {
        onPackSelectionChanged();
    }
}

FTBAPage::~FTBAPage()
{
    delete ui;
}

bool FTBAPage::shouldDisplay() const
{
    return true;
}

void FTBAPage::openedImpl()
{
    if(!initialized)
    {
        packModel->reload();
        initialized = true;
    }
    suggestCurrent();
}

void FTBAPage::suggestCurrent()
{
    if(!isOpened)
    {
        return;
    }

    if(!selected)
    {
        dialog->setSuggestedPack();
        return;
    }
    auto selectedPack = *selected;

    dialog->setSuggestedPack(selectedPack.name, new PackInstallTask(selectedPack));
    QString logoName = QString("ftb_%1").arg(selectedPack.id);
    dialog->setSuggestedIconFromFile(selectedPack.iconPath, logoName);
}

void FTBAPage::onPublicPackSelectionChanged(QModelIndex now, QModelIndex prev)
{
    if(!now.isValid())
    {
        onPackSelectionChanged();
        return;
    }
    Modpack selectedPack = packModel->data(now, Qt::UserRole).value<Modpack>();
    onPackSelectionChanged(&selectedPack);
}

void FTBAPage::onPackSelectionChanged(Modpack* pack)
{
    if(pack)
    {
        currentModpackInfo->setHtml("Pack by <b>" + pack->authors.join(", ") + "</b>" + "<br>Minecraft " + pack->mcVersion + "<br>" + "<br>" + pack->description);
        selected = *pack;
    }
    else
    {
        currentModpackInfo->setHtml("");
        if(isOpened)
        {
            dialog->setSuggestedPack();
        }
        return;
    }
    suggestCurrent();
}

}
