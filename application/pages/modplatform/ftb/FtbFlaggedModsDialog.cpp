#include "FtbFlaggedModsDialog.h"
#include "ui_FtbFlaggedModsDialog.h"

#include "FtbFlaggedModsListModel.h"

FtbFlaggedModsDialog::FtbFlaggedModsDialog(QWidget *parent, QVector<ModpacksCH::FlaggedMod> mods)
    : QDialog(parent), ui(new Ui::FtbFlaggedModsDialog)
{
    ui->setupUi(this);

    listModel = new FlaggedModsListModel(this, mods);

    ui->modTreeView->setModel(listModel);

    connect(this, &FtbFlaggedModsDialog::finished, listModel, &FlaggedModsListModel::enableDisableMods);
}

FtbFlaggedModsDialog::~FtbFlaggedModsDialog() {
    delete ui;
}
