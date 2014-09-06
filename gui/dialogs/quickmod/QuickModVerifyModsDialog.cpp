#include "QuickModVerifyModsDialog.h"
#include "ui_QuickModVerifyModsDialog.h"

QuickModVerifyModsDialog::QuickModVerifyModsDialog(QList<QuickModPtr> mods, QWidget *parent)
	: QDialog(parent), ui(new Ui::QuickModVerifyModsDialog)
{
	ui->setupUi(this);

	for (auto mod : mods)
	{
		new QTreeWidgetItem(ui->modsList, QStringList() << mod->name() << mod->repo()
														<< mod->updateUrl().toString());
	}
}

QuickModVerifyModsDialog::~QuickModVerifyModsDialog()
{
	delete ui;
}
