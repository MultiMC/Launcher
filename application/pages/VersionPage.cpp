/* Copyright 2013-2015 MultiMC Contributors
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

#include <pathutils.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QEvent>
#include <QKeyEvent>

#include "VersionPage.h"
#include "ui_VersionPage.h"

#include "Platform.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/VersionSelectDialog.h"
#include "dialogs/ModEditDialogCommon.h"

#include "dialogs/ProgressDialog.h"

#include <QAbstractItemModel>
#include <QMessageBox>
#include <QListView>
#include <QString>
#include <QUrl>

#include "tasks/Task.h"

#include "minecraft/MinecraftProfile.h"
#include "minecraft/Mod.h"
#include "icons/IconList.h"


QIcon VersionPage::icon() const
{
	return ENV.icons()->getIcon(m_inst->iconKey());
}
bool VersionPage::shouldDisplay() const
{
	return !m_inst->isRunning();
}

VersionPage::VersionPage(OneSixInstance *inst, QWidget *parent)
	: QWidget(parent), ui(new Ui::VersionPage), m_inst(inst)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	reloadMinecraftProfile();

	m_version = m_inst->getMinecraftProfile();
	if (m_version)
	{
		ui->libraryTreeView->setModel(m_version.get());
		ui->libraryTreeView->installEventFilter(this);
		ui->libraryTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
		connect(ui->libraryTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
				this, &VersionPage::versionCurrent);
		updateVersionControls();
		// select first item.
		auto index = ui->libraryTreeView->model()->index(0,0);
		if(index.isValid())
			ui->libraryTreeView->setCurrentIndex(index);
	}
	else
	{
		disableVersionControls();
	}
	connect(m_inst, &OneSixInstance::versionReloaded, this,
			&VersionPage::updateVersionControls);
}

VersionPage::~VersionPage()
{
	delete ui;
}

void VersionPage::updateVersionControls()
{
	ui->forgeBtn->setEnabled(true);
	ui->liteloaderBtn->setEnabled(true);
}

void VersionPage::disableVersionControls()
{
	ui->forgeBtn->setEnabled(false);
	ui->liteloaderBtn->setEnabled(false);
	ui->reloadLibrariesBtn->setEnabled(false);
	ui->removeLibraryBtn->setEnabled(false);
}

bool VersionPage::reloadMinecraftProfile()
{
	try
	{
		m_inst->reloadProfile();
		return true;
	}
	catch (Exception &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
		return false;
	}
	catch (...)
	{
		QMessageBox::critical(
			this, tr("Error"),
			tr("Couldn't load the instance profile."));
		return false;
	}
}

void VersionPage::on_reloadLibrariesBtn_clicked()
{
	reloadMinecraftProfile();
}

void VersionPage::on_removeLibraryBtn_clicked()
{
	if (ui->libraryTreeView->currentIndex().isValid())
	{
		// FIXME: use actual model, not reloading.
		if (!m_version->remove(ui->libraryTreeView->currentIndex().row()))
		{
			QMessageBox::critical(this, tr("Error"), tr("Couldn't remove file"));
		}
	}
}

void VersionPage::on_jarmodBtn_clicked()
{
	QFileDialog w;
	QSet<QString> locations;
	QString modsFolder = MMC->settings()->get("CentralModsDir").toString();
	auto f = [&](QStandardPaths::StandardLocation l)
	{
		QString location = QStandardPaths::writableLocation(l);
		QFileInfo finfo(location);
		if (!finfo.exists())
			return;
		locations.insert(location);
	};
	f(QStandardPaths::DesktopLocation);
	f(QStandardPaths::DocumentsLocation);
	f(QStandardPaths::DownloadLocation);
	f(QStandardPaths::HomeLocation);
	QList<QUrl> urls;
	for (auto location : locations)
	{
		urls.append(QUrl::fromLocalFile(location));
	}
	urls.append(QUrl::fromLocalFile(modsFolder));

	w.setFileMode(QFileDialog::ExistingFiles);
	w.setAcceptMode(QFileDialog::AcceptOpen);
	w.setNameFilter(tr("Minecraft jar mods (*.zip *.jar)"));
	w.setDirectory(modsFolder);
	w.setSidebarUrls(urls);

	if (w.exec())
		m_version->installJarMods(w.selectedFiles());
}

void VersionPage::on_moveLibraryUpBtn_clicked()
{
	if (ui->libraryTreeView->selectionModel()->selectedRows().isEmpty())
	{
		return;
	}
	try
	{
		const int row = ui->libraryTreeView->selectionModel()->selectedRows().first().row();
		m_version->move(row, MinecraftProfile::MoveUp);
	}
	catch (Exception &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
	}
}

void VersionPage::on_moveLibraryDownBtn_clicked()
{
	if (ui->libraryTreeView->selectionModel()->selectedRows().isEmpty())
	{
		return;
	}
	try
	{
		const int row = ui->libraryTreeView->selectionModel()->selectedRows().first().row();
		m_version->move(row, MinecraftProfile::MoveDown);
	}
	catch (Exception &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
	}
}

void VersionPage::on_changeVersionBtn_clicked()
{
	if(!ui->libraryTreeView->currentIndex().isValid())
		return;

	auto patch = m_version->versionPatch(ui->libraryTreeView->currentIndex().row());
	auto uid = patch->getPatchID();
	auto vlist = ENV.getVersionList(uid);
	if(!vlist)
		return;

	VersionSelectDialog vselect(vlist.get(), tr("Change %1 version").arg(patch->getPatchName()),
								this);
	if (!vselect.exec() || !vselect.selectedVersion())
		return;

	QString version = vselect.selectedVersion()->descriptor();
	qDebug() << "Version" << vselect.selectedVersion()->descriptor() << "selected for" << uid;

	if(uid == "net.minecraft")
	{
		m_inst->setMinecraftVersion(version);
	}
	else if(uid == "org.lwjgl")
	{
		m_inst->setLwjglVersion(version);
	}
	else if(uid == "net.minecraftforge")
	{
		m_inst->setForgeVersion(version);
	}
	else if(uid == "com.mumfrey.liteloader")
	{
		m_inst->setLiteloaderVersion(version);
	}
	else
	{
		qDebug() << "UID" << uid << "not allowed for OneSix";
		return;
	}
	auto updateTask = m_inst->doUpdate();
	if (!updateTask)
	{
		return;
	}
	ProgressDialog tDialog(this);
	connect(updateTask.get(), SIGNAL(failed(QString)), SLOT(onGameUpdateError(QString)));
	tDialog.exec(updateTask.get());
}

void VersionPage::on_forgeBtn_clicked()
{
	auto vlist = ENV.getVersionList("net.minecraftforge");
	if(!vlist)
		return;
	VersionSelectDialog vselect(vlist.get(), tr("Select Forge version"), this);
	if (vselect.exec() && vselect.selectedVersion())
	{
		ProgressDialog dialog(this);
		m_inst->setForgeVersion(vselect.selectedVersion()->descriptor());
		auto updateTask = m_inst->doUpdate();
		if (!updateTask)
		{
			return;
		}
		ProgressDialog tDialog(this);
		connect(updateTask.get(), SIGNAL(failed(QString)), SLOT(onGameUpdateError(QString)));
		tDialog.exec(updateTask.get());
	}
}

void VersionPage::on_liteloaderBtn_clicked()
{
	auto vlist = ENV.getVersionList("com.mumfrey.liteloader");
	if(!vlist)
		return;
	VersionSelectDialog vselect(vlist.get(), tr("Select LiteLoader version"), this);
	if (vselect.exec() && vselect.selectedVersion())
	{
		ProgressDialog dialog(this);
		m_inst->setLiteloaderVersion(vselect.selectedVersion()->descriptor());
		auto updateTask = m_inst->doUpdate();
		if (!updateTask)
		{
			return;
		}
		ProgressDialog tDialog(this);
		connect(updateTask.get(), SIGNAL(failed(QString)), SLOT(onGameUpdateError(QString)));
		tDialog.exec(updateTask.get());
	}
	// FIXME: add back filtering by mc version and the 'nice' message
	/*
	vselect.setExactFilter(BaseVersionList::ParentGameVersionRole, m_inst->currentVersionId());
	vselect.setEmptyString(tr("No LiteLoader versions are currently available for Minecraft ") +
						   m_inst->currentVersionId());
	*/
}

void VersionPage::versionCurrent(const QModelIndex &current, const QModelIndex &previous)
{
	if (!current.isValid())
	{
		ui->removeLibraryBtn->setDisabled(true);
		ui->moveLibraryDownBtn->setDisabled(true);
		ui->moveLibraryUpBtn->setDisabled(true);
	}
	else
	{
		bool enabled = m_version->canRemove(current.row());
		ui->removeLibraryBtn->setEnabled(enabled);
		ui->moveLibraryDownBtn->setEnabled(enabled);
		ui->moveLibraryUpBtn->setEnabled(enabled);
	}
	QString selectedId = m_version->versionFileId(current.row());
	if(ENV.getVersionList(selectedId))
	{
		ui->changeVersionBtn->setEnabled(true);
	}
	else
	{
		ui->changeVersionBtn->setEnabled(false);
	}
}

void VersionPage::onGameUpdateError(QString error)
{
	CustomMessageBox::selectable(this, tr("Error updating instance"), error,
								 QMessageBox::Warning)->show();
}
