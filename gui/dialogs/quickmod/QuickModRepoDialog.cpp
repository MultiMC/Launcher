#include "QuickModRepoDialog.h"
#include "ui_QuickModRepoDialog.h"

#include "logic/quickmod/QuickModsList.h"
#include "MultiMC.h"

QuickModRepoDialog::QuickModRepoDialog(QWidget *parent)
	: QDialog(parent), ui(new Ui::QuickModRepoDialog)
{
	ui->setupUi(this);
	ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

	populate();
}

QuickModRepoDialog::~QuickModRepoDialog()
{
	delete ui;
}

void QuickModRepoDialog::populate()
{
	QMap<QString, QTreeWidgetItem *> repos;

	auto list = MMC->quickmodslist();

	for (int i = 0; i < list->numMods(); ++i)
	{
		QuickModPtr mod = list->modAt(i);
		if (!repos.contains(mod->repo()))
		{
			QTreeWidgetItem *repo =
				new QTreeWidgetItem(ui->treeWidget, QStringList() << mod->repo());
			if (list->haveRepositoryIndexUrl(mod->repo()))
			{
				repo->setData(1, Qt::DisplayRole, list->repositoryIndexUrl(mod->repo()));
			}
			repos[mod->repo()] = repo;
		}
		QTreeWidgetItem *repo = repos[mod->repo()];
		QTreeWidgetItem *modItem =
			new QTreeWidgetItem(repo, QStringList() << mod->name() << mod->uid().toString());
	}
}
