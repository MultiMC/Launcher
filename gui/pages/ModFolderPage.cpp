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

#include "ModFolderPage.h"
#include "ui_ModFolderPage.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QEvent>
#include <QKeyEvent>
#include <QDesktopServices>
#include <QAbstractItemModel>
#include <QFileSystemModel>

#include <pathutils.h>

#include "MultiMC.h"
#include "gui/dialogs/CustomMessageBox.h"
#include "gui/dialogs/ModEditDialogCommon.h"
#include "gui/delegates/QuickModDelegate.h"
#include "logic/ModList.h"
#include "logic/Mod.h"
#include "logic/VersionFilterData.h"
#include "logic/quickmod/QuickModInstanceModList.h"
#include "logic/quickmod/QuickModsList.h"
#include "logic/tasks/SequentialTask.h"
#include "logic/KRecursiveFilterProxyModel.h"

bool listContainsSubstring(const QStringList &list, const QString &str)
{
	for (const QString &item : list)
	{
		if (item.contains(str, Qt::CaseInsensitive))
		{
			return true;
		}
	}
	return false;
}

class ModFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	ModFilterProxyModel(QObject *parent = 0) : QSortFilterProxyModel(parent)
	{
		setSortRole(Qt::DisplayRole);
		setSortCaseSensitivity(Qt::CaseInsensitive);
	}

	void setSourceModel(QAbstractItemModel *model)
	{
		QSortFilterProxyModel::setSourceModel(model);
		connect(model, &QAbstractItemModel::modelReset, this, &ModFilterProxyModel::resort);
		connect(model, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(resort()));
		resort();
	}

public slots:
	void setMCVersion(const QString &version)
	{
		m_mcVersion = version;
		invalidateFilter();
	}
	void setFulltext(const QString &query)
	{
		m_fulltext = query;
		invalidateFilter();
	}

private
slots:
	void resort()
	{
		sort(0);
	}

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
	{
		const QModelIndex index = sourceModel()->index(source_row, 0, source_parent);

		if (!m_mcVersion.isEmpty())
		{
			if (!listContainsSubstring(index.data(QuickModsList::MCVersionsRole).toStringList(),
									   m_mcVersion))
			{
				return false;
			}
		}
		if (!m_fulltext.isEmpty())
		{
			bool inName = index.data(QuickModsList::NameRole).toString().contains(
				m_fulltext, Qt::CaseInsensitive);
			bool inDesc = index.data(QuickModsList::DescriptionRole).toString().contains(
				m_fulltext, Qt::CaseInsensitive);
			if (!inName && !inDesc)
			{
				return false;
			}
		}

		return true;
	}

private:
	QString m_mcVersion;
	QString m_fulltext;
};


ModFolderPage::ModFolderPage(QuickModInstanceModList::Type type, BaseInstance *inst,
							 std::shared_ptr<ModList> mods, QString id, QString iconName,
							 QString displayName, QString helpPage, QWidget *parent)
	: QWidget(parent), ui(new Ui::ModFolderPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();
	m_inst = inst;
	m_mods = mods;
	m_id = id;
	m_displayName = displayName;
	m_iconName = iconName;
	m_helpName = helpPage;
	ui->modTreeView->installEventFilter(this);
	m_mods->startWatching();

	if (auto onesix = std::dynamic_pointer_cast<OneSixInstance>(inst->getSharedPtr()))
	{
		m_modsModel = new QuickModInstanceModList(type, onesix, m_mods, this);
		m_modsModel->bind("QuickMods.ConfirmRemoval", this,
						  &ModFolderPage::quickmodsConfirmRemoval);
		ui->modTreeView->setModel(m_proxy =
									  new QuickModInstanceModListProxy(m_modsModel, this));
	}
	else
	{
		ui->updateModBtn->setVisible(false);
		ui->modTreeView->setModel(m_mods.get());
	}

	// FIXME: because of the lazy loading of QFileSystemModel, KRecursiveFilterProxyModel can't
	// find stuff before the dir has been expanded once
	ensureFolderPathExists(MMC->settings()->get("CentralModsDir").toString());
	QFileSystemModel *fsModel = new QFileSystemModel(this);
	KRecursiveFilterProxyModel *fsFilterProxy = new KRecursiveFilterProxyModel(this);
	fsFilterProxy->setSourceModel(fsModel);
	ui->filesystemView->setModel(fsFilterProxy);
	ui->filesystemView->setRootIndex(fsFilterProxy->mapFromSource(
		fsModel->setRootPath(MMC->settings()->get("CentralModsDir").toString())));
	ui->filesystemView->setDragDropMode(QAbstractItemView::DragOnly);
	ui->filesystemView->setDefaultDropAction(Qt::CopyAction);
	ui->filesystemView->setDragEnabled(true);

	connect(ui->filesystemSearchEdit, &QLineEdit::textChanged, fsFilterProxy,
			&KRecursiveFilterProxyModel::setFilterWildcard);

	ModFilterProxyModel *qmFilterProxy = new ModFilterProxyModel(this);
	qmFilterProxy->setMCVersion(m_inst->intendedVersionId());
	qmFilterProxy->setSourceModel(MMC->quickmodslist().get());
	ui->quickmodsView->setModel(qmFilterProxy);
	// TODO this requires drag support in QuickModsList
	ui->quickmodsView->setDragDropMode(QAbstractItemView::DragOnly);
	ui->quickmodsView->setDefaultDropAction(Qt::CopyAction);
	ui->quickmodsView->setDragEnabled(true);
	ui->quickmodsView->setItemDelegate(new QuickModDelegate(ui->quickmodsView));

	connect(ui->quickmodsSearchEdit, &QLineEdit::textChanged, qmFilterProxy,
			&ModFilterProxyModel::setFulltext);

	auto smodel = ui->modTreeView->selectionModel();
	connect(smodel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
			SLOT(modCurrent(QModelIndex, QModelIndex)));

	updateOrphans();
}

ModFolderPage::~ModFolderPage()
{
	m_mods->stopWatching();
	delete ui;
}

bool ModFolderPage::shouldDisplay() const
{
	if (m_inst)
		return !m_inst->isRunning();
	return true;
}

bool ModFolderPage::modListFilter(QKeyEvent *keyEvent)
{
	switch (keyEvent->key())
	{
	case Qt::Key_Delete:
		on_rmModBtn_clicked();
		return true;
	case Qt::Key_Plus:
		on_filesystemBrowseBtn_clicked();
		return true;
	default:
		break;
	}
	return QWidget::eventFilter(ui->modTreeView, keyEvent);
}

bool ModFolderPage::eventFilter(QObject *obj, QEvent *ev)
{
	if (ev->type() != QEvent::KeyPress)
	{
		return QWidget::eventFilter(obj, ev);
	}
	QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
	if (obj == ui->modTreeView)
		return modListFilter(keyEvent);
	return QWidget::eventFilter(obj, ev);
}

void ModFolderPage::on_rmModBtn_clicked()
{
	int first, last;
	auto tmp = ui->modTreeView->selectionModel()->selectedRows();
	QModelIndexList quickmods, mods;
	sortMods(tmp, &quickmods, &mods);

	// mods
	{
		if (lastfirst(mods, first, last))
		{
			m_mods->stopWatching();
			m_mods->deleteMods(first, last);
			m_mods->startWatching();
		}
	}
	// quickmods
	if (!quickmods.isEmpty())
	{
		try
		{
			m_modsModel->removeMods(quickmods);
		}
		catch (MMCError &error)
		{
			QMessageBox::critical(this, tr("Error"),
								  tr("Unable to remove mod:\n%1").arg(error.cause()));
		}
		updateOrphans();
	}
}

void ModFolderPage::on_viewModBtn_clicked()
{
	openDirInDefaultProgram(m_mods->dir().absolutePath(), true);
}
void ModFolderPage::on_updateModBtn_clicked()
{
	auto tmp = ui->modTreeView->selectionModel()->selectedRows();
	QModelIndexList quickmods, mods;
	sortMods(tmp, &quickmods, &mods);
	try
	{
		if (!quickmods.isEmpty())
		{
			m_modsModel->updateMods(quickmods);
		}
	}
	catch (MMCError &error)
	{
		QMessageBox::critical(this, tr("Error"),
							  tr("Unable to update mod:\n%1").arg(error.cause()));
	}
	updateOrphans();
}

void ModFolderPage::on_orphansRemoveBtn_clicked()
{
	if (!ui->orphansBox->isVisible())
	{
		return;
	}
	OneSixInstance *onesix = dynamic_cast<OneSixInstance *>(m_inst);
	if (!onesix)
	{
		return;
	}
	onesix->removeQuickMods(m_orphans);
	updateOrphans();
}

void ModFolderPage::on_filesystemBrowseBtn_clicked()
{
	const QStringList fileNames = QFileDialog::getOpenFileNames(
		this, QApplication::translate("ModFolderPage", "Select Loader Mods"));
	for (auto filename : fileNames)
	{
		m_mods->stopWatching();
		m_mods->installMod(QFileInfo(filename));
		m_mods->startWatching();
	}
}

bool ModFolderPage::quickmodsConfirmRemoval(const QList<QuickModRef> &uids)
{
	QStringList names;
	for (const auto uid : uids)
	{
		names.append(uid.userFacing());
	}

	QMessageBox box;
	box.setParent(this, Qt::Dialog);
	box.setWindowTitle(tr("Continue?"));
	box.setIcon(QMessageBox::Warning);
	box.setText(tr("Remove these QuickMods?"));
	box.setInformativeText(
		tr("The QuickMods you selected for removal are depended on by some other mods, and "
		   "therefore those mods will also be removed. Continue?"));
	box.setDetailedText(names.join(", "));
	box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	box.setDefaultButton(QMessageBox::Yes);
	box.setEscapeButton(QMessageBox::No);
	return box.exec() == QMessageBox::Yes;
}

void ModFolderPage::updateOrphans()
{
	if (!m_modsModel)
	{
		return;
	}

	m_orphans = m_modsModel->findOrphans();
	QStringList names;
	for (const auto orphan : m_orphans)
	{
		names.append(orphan.userFacing());
	}
	ui->orphansLabel->setText(names.join(", "));

	ui->orphansBox->setVisible(!m_orphans.isEmpty() &&
							   m_modsModel->type() == QuickModInstanceModList::Mods);
}

void ModFolderPage::modCurrent(const QModelIndex &current, const QModelIndex &previous)
{
	if (m_modsModel)
	{
		ui->updateModBtn->setEnabled(
			!m_modsModel->isModListArea(m_proxy->mapToSource(current)));
	}

	const QModelIndex index = mapToModsList(current);
	if (!index.isValid())
	{
		ui->frame->clear();
		return;
	}
	int row = index.row();
	Mod &m = m_mods->operator[](row);
	ui->frame->updateWithMod(m);
}

QModelIndex ModFolderPage::mapToModsList(const QModelIndex &view) const
{
	return m_modsModel ? m_modsModel->mapToModList(m_proxy->mapToSource(view)) : view;
}
void ModFolderPage::sortMods(const QModelIndexList &view, QModelIndexList *quickmods,
							 QModelIndexList *mods)
{
	if (!m_modsModel)
	{
		*mods = view;
	}
	else
	{
		for (auto index : view)
		{
			if (!index.isValid())
			{
				continue;
			}
			const QModelIndex mappedOne = m_proxy->mapToSource(index);
			if (m_modsModel->isModListArea(mappedOne))
			{
				mods->append(m_modsModel->mapToModList(mappedOne));
			}
			else
			{
				quickmods->append(mappedOne);
			}
		}
	}
}

CoreModFolderPage::CoreModFolderPage(BaseInstance *inst, std::shared_ptr<ModList> mods,
									 QString id, QString iconName, QString displayName,
									 QString helpPage, QWidget *parent)
	: ModFolderPage(QuickModInstanceModList::CoreMods, inst, mods, id, iconName, displayName,
					helpPage, parent)
{
}
bool CoreModFolderPage::shouldDisplay() const
{
	if (ModFolderPage::shouldDisplay())
	{
		auto inst = dynamic_cast<OneSixInstance *>(m_inst);
		if (!inst)
			return true;
		auto version = inst->getFullVersion();
		if (!version)
			return true;
		if (version->m_releaseTime < g_VersionFilterData.legacyCutoffDate)
		{
			return true;
		}
	}
	return false;
}

#include "ModFolderPage.moc"
