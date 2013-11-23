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
void ChooseQuickModVersionDialog::setMod(const QuickMod *mod, const BaseInstance* instance, const QString &versionFilter)
{
	ui->versionsWidget->clear();

	for (int i = 0; i < mod->numVersions(); ++i)
	{
		const QuickMod::Version version = mod->version(i);
		QTreeWidgetItem* item = new QTreeWidgetItem(ui->versionsWidget);
		item->setText(0, version.name);
		item->setText(1, version.compatibleVersions.join(", "));
		item->setData(0, Qt::UserRole, i);
		if (version.compatibleVersions.contains(instance->currentVersionId()) && versionIsInFilter(version.name, versionFilter))
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

bool ChooseQuickModVersionDialog::versionIsInFilter(const QString &version, const QString &filter)
{
	if (filter.isEmpty())
	{
		return true;
	}

	/* We can have these versions of filters:
	 *
	 * X+ (matches version X and above)
	 * X -> Y (matches all versions between X and Y (inclusive))
	 * X (matches only version X)
	 *
	 * Matching itself is done using normal operators (<, >, <= and >=)
	 */

	if (filter.endsWith('+'))
	{
		const QString bottom = filter.left(filter.size() - 1);
		return bottom <= version;
	}
	else if (filter.contains(" -> "))
	{
		const QString bottom = filter.left(filter.indexOf(" -> "));
		const QString top = filter.mid(filter.indexOf(" -> ") + 4);
		return bottom <= version && version <= top;
	}
	else
	{
		return version == filter;
	}
}
