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

	// Interval notation is used
	QRegularExpression exp("(?<start>[\\[\\]\\(\\)])(?<bottom>.*?)(,(?<top>.*?))?(?<end>[\\[\\]\\(\\)])");
	QRegularExpressionMatch match = exp.match(filter);
	if (match.hasMatch())
	{
		const QChar start = match.captured("start").at(0);
		const QChar end = match.captured("end").at(0);
		const QString bottom = match.captured("bottom");
		const QString top = match.captured("top");

		// check if in range (bottom)
		if (!bottom.isEmpty())
		{
			if ((start == '[') && !(version >= bottom))
			{
				return false;
			}
			else if ((start == '(') && !(version > bottom))
			{
				return false;
			}
		}

		// check if in range (top)
		if (!top.isEmpty())
		{
			if ((end == ']') && !(version <= top))
			{
				return false;
			}
			else if ((end == ')') && !(version < top))
			{
				return false;
			}
		}

		return true;
	}
	else if (version != filter)
	{
		return false;
	}

	return true;
}
