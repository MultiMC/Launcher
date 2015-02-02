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

#include "gui/Platform.h"
#include "gui/dialogs/CustomMessageBox.h"
#include "gui/dialogs/VersionSelectDialog.h"
#include "gui/dialogs/ModEditDialogCommon.h"

#include "gui/dialogs/ProgressDialog.h"

#include <QAbstractItemModel>
#include <QMessageBox>
#include <QListView>
#include <QString>
#include <QUrl>

#include "logic/ModList.h"
#include "logic/minecraft/InstanceVersion.h"
#include "logic/EnabledItemFilter.h"
#include "logic/forge/ForgeVersionList.h"
#include "logic/forge/ForgeInstaller.h"
#include "logic/liteloader/LiteLoaderVersionList.h"
#include "logic/liteloader/LiteLoaderInstaller.h"
#include "logic/minecraft/VersionBuilder.h"
#include "logic/auth/MojangAccountList.h"
#include "logic/Mod.h"
#include "logic/icons/IconList.h"


QIcon VersionPage::icon() const
{
	return MMC->icons()->getIcon(m_inst->iconKey());
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

	m_version = m_inst->getFullVersion();
	if (m_version)
	{
		main_model = new EnabledItemFilter(this);
		main_model->setActive(true);
		main_model->setSourceModel(m_version.get());
		ui->libraryTreeView->setModel(main_model);
		ui->libraryTreeView->installEventFilter(this);
		ui->libraryTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
		connect(ui->libraryTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
				this, &VersionPage::versionCurrent);
		updateVersionControls();
		// select first item.
		auto index = main_model->index(0,0);
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

bool VersionPage::reloadInstanceVersion()
{
	try
	{
		m_inst->reloadVersion();
		return true;
	}
	catch (MMCError &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
		return false;
	}
	catch (...)
	{
		QMessageBox::critical(
			this, tr("Error"),
			tr("Failed to load the version description file for reasons unknown."));
		return false;
	}
}

void VersionPage::on_reloadLibrariesBtn_clicked()
{
	reloadInstanceVersion();
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

void VersionPage::on_resetLibraryOrderBtn_clicked()
{
	try
	{
		m_version->resetOrder();
	}
	catch (MMCError &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
	}
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
		m_version->move(row, InstanceVersion::MoveUp);
	}
	catch (MMCError &e)
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
		m_version->move(row, InstanceVersion::MoveDown);
	}
	catch (MMCError &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
	}
}

void VersionPage::on_changeMCVersionBtn_clicked()
{
	VersionSelectDialog vselect(m_inst->versionList().get(), tr("Change Minecraft version"),
								this);
	if (!vselect.exec() || !vselect.selectedVersion())
		return;

	if (!MMC->accounts()->anyAccountIsValid())
	{
		CustomMessageBox::selectable(
			this, tr("Error"),
			tr("MultiMC cannot download Minecraft or update instances unless you have at least "
			   "one account added.\nPlease add your Mojang or Minecraft account."),
			QMessageBox::Warning)->show();
		return;
	}

	if (m_inst->versionIsCustom())
	{
		auto result = CustomMessageBox::selectable(
			this, tr("Are you sure?"),
			tr("This will remove any library/version customization you did previously. "
			   "This includes things like Forge install and similar."),
			QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Abort,
			QMessageBox::Abort)->exec();

		if (result != QMessageBox::Ok)
			return;
		m_version->revertToVanilla();
		reloadInstanceVersion();
	}
	m_inst->setIntendedVersionId(vselect.selectedVersion()->descriptor());

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
	// FIXME: use actual model, not reloading. Move logic to model.
	if (m_version->hasFtbPack())
	{
		if (QMessageBox::question(
				this, tr("Revert?"),
				tr("This action will remove the FTB pack version patch. Continue?")) !=
			QMessageBox::Yes)
		{
			return;
		}
		m_version->removeFtbPack();
		reloadInstanceVersion();
	}
	if (m_version->hasDeprecatedVersionFiles())
	{
		if (QMessageBox::question(this, tr("Revert?"),
								  tr("This action will remove deprecated version files "
									 "(custom.json and version.json). Continue?")) !=
			QMessageBox::Yes)
		{
			return;
		}
		m_version->removeDeprecatedVersionFiles();
		reloadInstanceVersion();
	}
	VersionSelectDialog vselect(MMC->forgelist().get(), tr("Select Forge version"), this);
	vselect.setExactFilter(1, m_inst->currentVersionId());
	vselect.setEmptyString(tr("No Forge versions are currently available for Minecraft ") +
						   m_inst->currentVersionId());
	if (vselect.exec() && vselect.selectedVersion())
	{
		ProgressDialog dialog(this);
		dialog.exec(
			ForgeInstaller().createInstallTask(m_inst, vselect.selectedVersion(), this));
	}
}

void VersionPage::on_liteloaderBtn_clicked()
{
	if (m_version->hasFtbPack())
	{
		if (QMessageBox::question(
				this, tr("Revert?"),
				tr("This action will remove the FTB pack version patch. Continue?")) !=
			QMessageBox::Yes)
		{
			return;
		}
		m_version->removeFtbPack();
		reloadInstanceVersion();
	}
	if (m_version->hasDeprecatedVersionFiles())
	{
		if (QMessageBox::question(this, tr("Revert?"),
								  tr("This action will remove deprecated version files "
									 "(custom.json and version.json). Continue?")) !=
			QMessageBox::Yes)
		{
			return;
		}
		m_version->removeDeprecatedVersionFiles();
		reloadInstanceVersion();
	}
	VersionSelectDialog vselect(MMC->liteloaderlist().get(), tr("Select LiteLoader version"),
								this);
	vselect.setExactFilter(1, m_inst->currentVersionId());
	vselect.setEmptyString(tr("No LiteLoader versions are currently available for Minecraft ") +
						   m_inst->currentVersionId());
	if (vselect.exec() && vselect.selectedVersion())
	{
		ProgressDialog dialog(this);
		dialog.exec(
			LiteLoaderInstaller().createInstallTask(m_inst, vselect.selectedVersion(), this));
	}
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
	if (selectedId == "net.minecraft" || selectedId == "org.multimc.custom.json" ||
		selectedId == "org.multimc.version.json")
	{
		ui->changeMCVersionBtn->setEnabled(true);
	}
	else
	{
		ui->changeMCVersionBtn->setEnabled(false);
	}
}
