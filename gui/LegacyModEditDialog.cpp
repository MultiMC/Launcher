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

#include "MultiMC.h"
#include "LegacyModEditDialog.h"
#include "ModEditDialogCommon.h"
#include "versionselectdialog.h"
#include "ProgressDialog.h"
#include "ui_LegacyModEditDialog.h"
#include "logic/ModList.h"
#include "logic/lists/ForgeVersionList.h"
#include "gui/platform.h"

#include <pathutils.h>
#include <QFileDialog>
//#include <QMessageBox>
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>

LegacyModEditDialog::LegacyModEditDialog(LegacyInstance *inst, QWidget *parent)
	: m_inst(inst), QDialog(parent), ui(new Ui::LegacyModEditDialog)
{
    MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);

	// Jar mods
	{
		ensureFolderPathExists(m_inst->jarModsDir());
		m_jarmods = m_inst->jarModList();
		ui->jarModsTreeView->setModel(m_jarmods.get());
#ifndef Q_OS_LINUX
		// FIXME: internal DnD causes segfaults later
		ui->jarModsTreeView->setDragDropMode(QAbstractItemView::DragDrop);
		// FIXME: DnD is glitched with contiguous (we move only first item in selection)
		ui->jarModsTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
#endif
		ui->jarModsTreeView->installEventFilter(this);
		m_jarmods->startWatching();
		auto smodel = ui->jarModsTreeView->selectionModel();
		connect(smodel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
				SLOT(jarCurrent(QModelIndex, QModelIndex)));
	}
	// Core mods
	{
		ensureFolderPathExists(m_inst->coreModsDir());
		m_coremods = m_inst->coreModList();
		ui->coreModsTreeView->setModel(m_coremods.get());
		ui->coreModsTreeView->installEventFilter(this);
		m_coremods->startWatching();
		auto smodel = ui->coreModsTreeView->selectionModel();
		connect(smodel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
				SLOT(coreCurrent(QModelIndex, QModelIndex)));
	}
	// Loader mods
	{
		ensureFolderPathExists(m_inst->loaderModsDir());
		m_mods = m_inst->loaderModList();
		ui->loaderModTreeView->setModel(m_mods.get());
		ui->loaderModTreeView->installEventFilter(this);
		m_mods->startWatching();
		auto smodel = ui->loaderModTreeView->selectionModel();
		connect(smodel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
				SLOT(loaderCurrent(QModelIndex, QModelIndex)));
	}
	// texture packs
	{
		ensureFolderPathExists(m_inst->texturePacksDir());
		m_texturepacks = m_inst->texturePackList();
		ui->texPackTreeView->setModel(m_texturepacks.get());
		ui->texPackTreeView->installEventFilter(this);
		m_texturepacks->startWatching();
	}
}

LegacyModEditDialog::~LegacyModEditDialog()
{
	m_mods->stopWatching();
	m_coremods->stopWatching();
	m_jarmods->stopWatching();
	m_texturepacks->stopWatching();
	delete ui;
}

bool LegacyModEditDialog::coreListFilter(QKeyEvent *keyEvent)
{
	switch (keyEvent->key())
	{
	case Qt::Key_Delete:
		on_rmCoreBtn_clicked();
		return true;
	case Qt::Key_Plus:
		on_addCoreBtn_clicked();
		return true;
	default:
		break;
	}
	return QDialog::eventFilter(ui->coreModsTreeView, keyEvent);
}

bool LegacyModEditDialog::jarListFilter(QKeyEvent *keyEvent)
{
	switch (keyEvent->key())
	{
	case Qt::Key_Up:
	{
		if (keyEvent->modifiers() & Qt::ControlModifier)
		{
			on_moveJarUpBtn_clicked();
			return true;
		}
		break;
	}
	case Qt::Key_Down:
	{
		if (keyEvent->modifiers() & Qt::ControlModifier)
		{
			on_moveJarDownBtn_clicked();
			return true;
		}
		break;
	}
	case Qt::Key_Delete:
		on_rmJarBtn_clicked();
		return true;
	case Qt::Key_Plus:
		on_addJarBtn_clicked();
		return true;
	default:
		break;
	}
	return QDialog::eventFilter(ui->jarModsTreeView, keyEvent);
}

bool LegacyModEditDialog::loaderListFilter(QKeyEvent *keyEvent)
{
	switch (keyEvent->key())
	{
	case Qt::Key_Delete:
		on_rmModBtn_clicked();
		return true;
	case Qt::Key_Plus:
		on_addModBtn_clicked();
		return true;
	default:
		break;
	}
	return QDialog::eventFilter(ui->loaderModTreeView, keyEvent);
}

bool LegacyModEditDialog::texturePackListFilter(QKeyEvent *keyEvent)
{
	switch (keyEvent->key())
	{
	case Qt::Key_Delete:
		on_rmTexPackBtn_clicked();
		return true;
	case Qt::Key_Plus:
		on_addTexPackBtn_clicked();
		return true;
	default:
		break;
	}
	return QDialog::eventFilter(ui->texPackTreeView, keyEvent);
}

bool LegacyModEditDialog::eventFilter(QObject *obj, QEvent *ev)
{
	if (ev->type() != QEvent::KeyPress)
	{
		return QDialog::eventFilter(obj, ev);
	}
	QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
	if (obj == ui->jarModsTreeView)
		return jarListFilter(keyEvent);
	if (obj == ui->coreModsTreeView)
		return coreListFilter(keyEvent);
	if (obj == ui->loaderModTreeView)
		return loaderListFilter(keyEvent);
	if (obj == ui->texPackTreeView)
		return texturePackListFilter(keyEvent);
	return QDialog::eventFilter(obj, ev);
}

void LegacyModEditDialog::on_addCoreBtn_clicked()
{
	//: Title of core mod selection dialog
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select Core Mods"));
	for (auto filename : fileNames)
	{
		m_coremods->stopWatching();
		m_coremods->installMod(QFileInfo(filename));
		m_coremods->startWatching();
	}
}
void LegacyModEditDialog::on_addForgeBtn_clicked()
{
	VersionSelectDialog vselect(MMC->forgelist().get(), tr("Select Forge version"), this);
	vselect.setFilter(1, m_inst->intendedVersionId());
	if (vselect.exec() && vselect.selectedVersion())
	{
		ForgeVersionPtr forge =
			std::dynamic_pointer_cast<ForgeVersion>(vselect.selectedVersion());
		if (!forge)
			return;
		auto entry = MMC->metacache()->resolveEntry("minecraftforge", forge->filename);
		if (entry->stale)
		{
			NetJob *fjob = new NetJob("Forge download");
			fjob->addNetAction(CacheDownload::make(forge->universal_url, entry));
			ProgressDialog dlg(this);
			dlg.exec(fjob);
			if (dlg.result() == QDialog::Accepted)
			{
				m_jarmods->stopWatching();
				m_jarmods->installMod(QFileInfo(entry->getFullPath()));
				m_jarmods->startWatching();
			}
			else
			{
				// failed to download forge :/
			}
		}
		else
		{
			m_jarmods->stopWatching();
			m_jarmods->installMod(QFileInfo(entry->getFullPath()));
			m_jarmods->startWatching();
		}
	}
}
void LegacyModEditDialog::on_addJarBtn_clicked()
{
	//: Title of jar mod selection dialog
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select Jar Mods"));
	for (auto filename : fileNames)
	{
		m_jarmods->stopWatching();
		m_jarmods->installMod(QFileInfo(filename));
		m_jarmods->startWatching();
	}
}
void LegacyModEditDialog::on_addModBtn_clicked()
{
	//: Title of regular mod selection dialog
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select Loader Mods"));
	for (auto filename : fileNames)
	{
		m_mods->stopWatching();
		m_mods->installMod(QFileInfo(filename));
		m_mods->startWatching();
	}
}
void LegacyModEditDialog::on_addTexPackBtn_clicked()
{
	//: Title of texture pack selection dialog
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select Texture Packs"));
	for (auto filename : fileNames)
	{
		m_texturepacks->stopWatching();
		m_texturepacks->installMod(QFileInfo(filename));
		m_texturepacks->startWatching();
	}
}

void LegacyModEditDialog::on_moveJarDownBtn_clicked()
{
	int first, last;
	auto list = ui->jarModsTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;

	m_jarmods->moveModsDown(first, last);
}
void LegacyModEditDialog::on_moveJarUpBtn_clicked()
{
	int first, last;
	auto list = ui->jarModsTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_jarmods->moveModsUp(first, last);
}
void LegacyModEditDialog::on_rmCoreBtn_clicked()
{
	int first, last;
	auto list = ui->coreModsTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_coremods->stopWatching();
	m_coremods->deleteMods(first, last);
	m_coremods->startWatching();
}
void LegacyModEditDialog::on_rmJarBtn_clicked()
{
	int first, last;
	auto list = ui->jarModsTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_jarmods->stopWatching();
	m_jarmods->deleteMods(first, last);
	m_jarmods->startWatching();
}
void LegacyModEditDialog::on_rmModBtn_clicked()
{
	int first, last;
	auto list = ui->loaderModTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_mods->stopWatching();
	m_mods->deleteMods(first, last);
	m_mods->startWatching();
}
void LegacyModEditDialog::on_rmTexPackBtn_clicked()
{
	int first, last;
	auto list = ui->texPackTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_texturepacks->stopWatching();
	m_texturepacks->deleteMods(first, last);
	m_texturepacks->startWatching();
}
void LegacyModEditDialog::on_viewCoreBtn_clicked()
{
	openDirInDefaultProgram(m_inst->coreModsDir(), true);
}
void LegacyModEditDialog::on_viewModBtn_clicked()
{
	openDirInDefaultProgram(m_inst->loaderModsDir(), true);
}
void LegacyModEditDialog::on_viewTexPackBtn_clicked()
{
	openDirInDefaultProgram(m_inst->texturePacksDir(), true);
}

void LegacyModEditDialog::on_buttonBox_rejected()
{
	close();
}

void LegacyModEditDialog::jarCurrent(QModelIndex current, QModelIndex previous)
{
	if(!current.isValid())
	{
		ui->jarMIFrame->clear();
		return;
	}
	int row = current.row();
	Mod &m = m_jarmods->operator[](row);
	ui->jarMIFrame->updateWithMod(m);
}

void LegacyModEditDialog::coreCurrent(QModelIndex current, QModelIndex previous)
{
	if(!current.isValid())
	{
		ui->coreMIFrame->clear();
		return;
	}
	int row = current.row();
	Mod &m = m_coremods->operator[](row);
	ui->coreMIFrame->updateWithMod(m);
}

void LegacyModEditDialog::loaderCurrent(QModelIndex current, QModelIndex previous)
{
	if(!current.isValid())
	{
		ui->loaderMIFrame->clear();
		return;
	}
	int row = current.row();
	Mod &m = m_mods->operator[](row);
	ui->loaderMIFrame->updateWithMod(m);
}
