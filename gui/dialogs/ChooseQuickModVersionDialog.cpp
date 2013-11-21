#include "ChooseQuickModVersionDialog.h"
#include "ui_ChooseQuickModVersionDialog.h"

#include <QDebug>

#include "logic/lists/QuickModsList.h"
#include "logic/BaseInstance.h"

ChooseQuickModVersionDialog::ChooseQuickModVersionDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ChooseQuickModVersionDialog)
{
	ui->setupUi(this);
}

ChooseQuickModVersionDialog::~ChooseQuickModVersionDialog()
{
	delete ui;
}

void ChooseQuickModVersionDialog::setCanCancel(bool canCancel)
{
	ui->cancelButton->setEnabled(canCancel);
}
void ChooseQuickModVersionDialog::setMod(const QuickMod *mod, const BaseInstance* instance)
{
	ui->versionsWidget->clear();

	for (int i = 0; i < mod->numVersions(); ++i)
	{
		const QuickMod::Version version = mod->version(i);
		QTreeWidgetItem* item = new QTreeWidgetItem(ui->versionsWidget);
		item->setText(0, version.name);
		item->setText(1, version.compatibleVersions.join(", "));
		item->setData(0, Qt::UserRole, i);
		if (version.compatibleVersions.contains(instance->currentVersionId()))
		{
			item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		}
		else
		{
			item->setFlags(Qt::NoItemFlags);
		}
	}

	ui->versionsWidget->sortByColumn(0);
}

int ChooseQuickModVersionDialog::version() const
{
	return ui->versionsWidget->selectedItems().first()->data(0, Qt::UserRole).toInt();
}
