/* Copyright 2014 MultiMC Contributors
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

#include "QuickModDownloadSelectionDialog.h"
#include "ui_QuickModDownloadSelectionDialog.h"

#include "logic/quickmod/QuickModVersion.h"
#include "logic/settings/SettingsObject.h"
#include "MultiMC.h"

QuickModDownloadSelectionDialog::QuickModDownloadSelectionDialog(
	const QuickModVersionPtr version, QWidget *parent)
	: QDialog(parent), ui(new Ui::QuickModDownloadSelectionDialog), m_version(version)
{
	ui->setupUi(this);

	bool haveEncoded = false;

	for (const auto download : version->downloads)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem(ui->list);
		switch (download.type)
		{
		case QuickModDownload::Direct:
			item->setText(0, tr("Direct"));
		case QuickModDownload::Maven:
			item->setText(0, tr("Direct (Maven)"));
		case QuickModDownload::Sequential:
			item->setText(0, tr("Sequential"));
		case QuickModDownload::Parallel:
			item->setText(0, tr("Parallel"));
		case QuickModDownload::Encoded:
			item->setText(0, tr("Encoded"));
		}

		item->setText(1, QString::number(download.priority));
		item->setText(2, download.url);
		if (download.type == QuickModDownload::Encoded)
		{
			item->setText(3, download.hint);
			item->setText(4, download.group);
			haveEncoded = true;
		}
	}

	if (!haveEncoded)
	{
		ui->list->setColumnCount(3);
	}
}

int QuickModDownloadSelectionDialog::selectedIndex() const
{
	return ui->list->currentIndex().row();
}

QuickModDownload
QuickModDownloadSelectionDialog::highestPriorityDownload(const QuickModVersionPtr version,
														 const int type)
{
	Q_ASSERT(!version->downloads.isEmpty());
	QuickModDownload current;
	current.priority = -1;
	for (auto download : version->downloads)
	{
		if (type != 0 && download.type != type)
		{
			continue;
		}
		if (download.priority > current.priority)
		{
			current = download;
		}
	}
	// if we didn't find one that matches
	if (current.priority == -1)
	{
		switch (type)
		{
		case QuickModDownload::Direct:
			return highestPriorityDownload(version, QuickModDownload::Maven);
		case QuickModDownload::Maven:
			return highestPriorityDownload(version);
		case QuickModDownload::Sequential:
			return highestPriorityDownload(version, QuickModDownload::Parallel);
		case QuickModDownload::Parallel:
			return highestPriorityDownload(version);
		case QuickModDownload::Encoded:
			return highestPriorityDownload(version, QuickModDownload::Direct);
		}
	}
	return current;
}

QuickModDownloadSelectionDialog::~QuickModDownloadSelectionDialog()
{
	delete ui;
}

QuickModDownload QuickModDownloadSelectionDialog::select(const QuickModVersionPtr version,
														 QWidget *widgetParent)
{
	if (version->downloads.isEmpty())
	{
		throw MMCError(tr("No downloads available"));
	}
	if (version->downloads.size() == 1)
	{
		return version->downloads.first();
	}

	const QString setting = MMC->settings()->get("QuickModDownloadSelection").toString();
	if (setting == "ask")
	{
		QuickModDownloadSelectionDialog dialog(version, widgetParent);
		if (dialog.exec() == QDialog::Accepted && dialog.selectedIndex() >= 0)
		{
			return version->downloads.at(dialog.selectedIndex());
		}
		QLOG_INFO() << "You didn't select a download, the one with the highest priority will "
					   "be used as a fall back";
		return highestPriorityDownload(version);
	}
	else if (setting == "priority")
	{
		return highestPriorityDownload(version);
	}
	else if (setting == "direct")
	{
		return highestPriorityDownload(version, QuickModDownload::Direct);
	}
	else if (setting == "sequential")
	{
		return highestPriorityDownload(version, QuickModDownload::Sequential);
	}
	else
	{
		// silently return the highest priority one
		return highestPriorityDownload(version);
	}
}
