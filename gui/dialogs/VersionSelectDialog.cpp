/* Copyright 2013 MultiMC Contributors
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

#include "VersionSelectDialog.h"
#include "ui_VersionSelectDialog.h"

#include <QHeaderView>

#include <QDebug>

#include <gui/dialogs/ProgressDialog.h>
#include "gui/Platform.h"

#include <logic/BaseVersion.h>
#include <logic/lists/BaseVersionList.h>
#include <logic/tasks/Task.h>

VersionSelectProxyModel::VersionSelectProxyModel(QObject *parent)
	: QSortFilterProxyModel(parent)
{
}

bool VersionSelectProxyModel::filterAcceptsRow(int source_row,
											   const QModelIndex &source_parent) const
{
	const QString version = sourceModel()->index(source_row, m_column).data().toString();

	bool ret = versionIsInFilter(version, m_filter);
	return ret;
}
QString VersionSelectProxyModel::filter() const
{
	return m_filter;
}
void VersionSelectProxyModel::setFilter(const QString &filter)
{
	m_filter = filter;
}
int VersionSelectProxyModel::column() const
{
	return m_column;
}
void VersionSelectProxyModel::setColumn(int column)
{
	m_column = column;
}

bool VersionSelectProxyModel::versionIsInFilter(const QString &version, const QString &filter)
{
	if (filter.isEmpty())
	{
		return true;
	}
	else if (version == filter)
	{
		return true;
	}

	// Interval notation is used
	QRegularExpression exp(
		"(?<start>[\\[\\]\\(\\)])(?<bottom>.*?)(,(?<top>.*?))?(?<end>[\\[\\]\\(\\)])");
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

	return false;
}

VersionSelectDialog::VersionSelectDialog(BaseVersionList *vlist, QString title, QWidget *parent,
										 bool cancelable)
	: QDialog(parent), ui(new Ui::VersionSelectDialog)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	setWindowModality(Qt::WindowModal);
	setWindowTitle(title);

	m_vlist = vlist;

	m_proxyModel = new VersionSelectProxyModel(this);
	m_proxyModel->setSourceModel(vlist);

	ui->listView->setModel(m_proxyModel);
	ui->listView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);

	if (!cancelable)
	{
		ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
	}
}

VersionSelectDialog::~VersionSelectDialog()
{
	delete ui;
}

void VersionSelectDialog::setResizeOn(int column)
{
	ui->listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::ResizeToContents);
	resizeOnColumn = column;
	ui->listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);
}

int VersionSelectDialog::exec()
{
	QDialog::open();
	if (!m_vlist->isLoaded())
	{
		loadList();
	}
	if (m_proxyModel->rowCount() == 0)
	{
		return QDialog::Rejected;
	}
	if (m_proxyModel->rowCount() == 1)
	{
		ui->listView->selectionModel()->setCurrentIndex(m_proxyModel->index(0, 0),
														QItemSelectionModel::ClearAndSelect);
		return QDialog::Accepted;
	}
	return QDialog::exec();
}

void VersionSelectDialog::loadList()
{
	ProgressDialog *taskDlg = new ProgressDialog(this);
	Task *loadTask = m_vlist->getLoadTask();
	loadTask->setParent(taskDlg);
	taskDlg->exec(loadTask);
	delete taskDlg; // without this the dialog won't dissapear?
}

BaseVersionPtr VersionSelectDialog::selectedVersion() const
{
	auto currentIndex = ui->listView->selectionModel()->currentIndex();
	auto variant = m_proxyModel->data(currentIndex, BaseVersionList::VersionPointerRole);
	return variant.value<BaseVersionPtr>();
}

void VersionSelectDialog::on_refreshButton_clicked()
{
	loadList();
}

void VersionSelectDialog::setFilter(int column, QString filter)
{
	m_proxyModel->setColumn(column);
	m_proxyModel->setFilter(filter);
	/*
	QStringList filteredTypes;
	if (!ui->filterSnapshotsCheckbox->isChecked())
		filteredTypes += "Snapshot";
	if (!ui->filterMCNostalgiaCheckbox->isChecked())
		filteredTypes += "Nostalgia";

	QString regexStr = "^.*$";
	if (filteredTypes.length() > 0)
		regexStr = QString("^((?!%1).)*$").arg(filteredTypes.join('|'));

	QLOG_DEBUG() << "Filter:" << regexStr;
	*/
}
