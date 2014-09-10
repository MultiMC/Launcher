#include "QuickModRepoDialog.h"
#include "ui_QuickModRepoDialog.h"

#include "logic/quickmod/QuickModsList.h"
#include "logic/quickmod/QuickModIndexList.h"
#include "MultiMC.h"

QuickModRepoDialog::QuickModRepoDialog(QWidget *parent)
	: QDialog(parent), ui(new Ui::QuickModRepoDialog)
{
	ui->setupUi(this);
	ui->treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->treeView->setSelectionMode(QTreeView::MultiSelection);
	ui->treeView->setSelectionBehavior(QTreeView::SelectRows);

	m_list = new QuickModIndexList(this);
}

QuickModRepoDialog::~QuickModRepoDialog()
{
	delete ui;
}

void QuickModRepoDialog::on_removeBtn_clicked()
{
	QModelIndexList indexes = ui->treeView->selectionModel()->selectedRows();
	QList<QuickModMetadataPtr> mods;
	for (const auto index : indexes)
	{
		mods.append(MMC->quickmodslist()->mod(index.data(Qt::UserRole).toString()));
	}
	mods.removeAll(nullptr);
	for (const auto mod : mods)
	{
		MMC->quickmodslist()->unregisterMod(mod);
	}
	m_list->reload();
}
