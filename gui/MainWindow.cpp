/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Andrew Okin
 *          Peterix
 *          Orochimarufan <orochimarufan.x3@gmail.com>
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

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "keyring.h"

#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>

#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QToolButton>

#include "osutils.h"
#include "userutils.h"
#include "pathutils.h"

#include "categorizedview.h"
#include "categorydrawer.h"

#include "gui/Platform.h"

#include "gui/widgets/InstanceDelegate.h"
#include "gui/widgets/LabeledToolButton.h"

#include "gui/dialogs/SettingsDialog.h"
#include "gui/dialogs/NewInstanceDialog.h"
#include "gui/dialogs/LoginDialog.h"
#include "gui/dialogs/ProgressDialog.h"
#include "gui/dialogs/AboutDialog.h"
#include "gui/dialogs/VersionSelectDialog.h"
#include "gui/dialogs/CustomMessageBox.h"
#include "gui/dialogs/LwjglSelectDialog.h"
#include "gui/dialogs/InstanceSettings.h"
#include "gui/dialogs/IconPickerDialog.h"
#include "gui/dialogs/EditNotesDialog.h"
#include "gui/dialogs/CopyInstanceDialog.h"
#include "gui/dialogs/AccountListDialog.h"

#include "gui/ConsoleWindow.h"

#include "logic/lists/InstanceList.h"
#include "logic/lists/MinecraftVersionList.h"
#include "logic/lists/LwjglVersionList.h"
#include "logic/lists/IconList.h"
#include "logic/lists/JavaVersionList.h"

#include "logic/net/LoginTask.h"

#include "logic/BaseInstance.h"
#include "logic/InstanceFactory.h"
#include "logic/MinecraftProcess.h"
#include "logic/OneSixAssets.h"
#include "logic/OneSixUpdate.h"
#include "logic/JavaUtils.h"
#include "logic/NagUtils.h"
#include "logic/SkinUtils.h"

#include "logic/LegacyInstance.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	setWindowTitle(QString("MultiMC %1").arg(MMC->version().toString()));

	// OSX magic.
	setUnifiedTitleAndToolBarOnMac(true);

	// The instance action toolbar customizations
	{
		// disabled until we have an instance selected
		ui->instanceToolBar->setEnabled(false);

		// the rename label is inside the rename tool button
		renameButton = new LabeledToolButton();
		renameButton->setText("Instance Name");
		renameButton->setToolTip(ui->actionRenameInstance->toolTip());
		connect(renameButton, SIGNAL(clicked(bool)), SLOT(on_actionRenameInstance_triggered()));
		ui->instanceToolBar->insertWidget(ui->actionLaunchInstance, renameButton);
		ui->instanceToolBar->insertSeparator(ui->actionLaunchInstance);
		renameButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	}

	// Create the instance list widget
	{
		view = new KCategorizedView(ui->centralWidget);
		drawer = new KCategoryDrawer(view);

		view->setSelectionMode(QAbstractItemView::SingleSelection);
		view->setCategoryDrawer(drawer);
		view->setCollapsibleBlocks(true);
		view->setViewMode(QListView::IconMode);
		view->setFlow(QListView::LeftToRight);
		view->setWordWrap(true);
		view->setMouseTracking(true);
		view->viewport()->setAttribute(Qt::WA_Hover);
		auto delegate = new ListViewDelegate();
		view->setItemDelegate(delegate);
		view->setSpacing(10);
		view->setUniformItemWidths(true);

		// do not show ugly blue border on the mac
		view->setAttribute(Qt::WA_MacShowFocusRect, false);

		view->installEventFilter(this);

		proxymodel = new InstanceProxyModel(this);
		//		proxymodel->setSortRole(KCategorizedSortFilterProxyModel::CategorySortRole);
		// proxymodel->setFilterRole(KCategorizedSortFilterProxyModel::CategorySortRole);
		// proxymodel->setDynamicSortFilter ( true );

		// FIXME: instList should be global-ish, or at least not tied to the main window...
		// maybe the application itself?
		proxymodel->setSourceModel(MMC->instances().get());
		proxymodel->sort(0);
		view->setFrameShape(QFrame::NoFrame);
		view->setModel(proxymodel);

		ui->horizontalLayout->addWidget(view);
	}
	// The cat background
	{
		bool cat_enable = MMC->settings()->get("TheCat").toBool();
		ui->actionCAT->setChecked(cat_enable);
		connect(ui->actionCAT, SIGNAL(toggled(bool)), SLOT(onCatToggled(bool)));
		setCatBackground(cat_enable);
	}
	// start instance when double-clicked
	connect(view, SIGNAL(doubleClicked(const QModelIndex &)), this,
			SLOT(instanceActivated(const QModelIndex &)));
	// track the selection -- update the instance toolbar
	connect(view->selectionModel(),
			SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this,
			SLOT(instanceChanged(const QModelIndex &, const QModelIndex &)));
	// model reset -> selection is invalid. All the instance pointers are wrong.
	// FIXME: stop using POINTERS everywhere
	connect(MMC->instances().get(), SIGNAL(dataIsInvalid()), SLOT(selectionBad()));

	m_statusLeft = new QLabel(tr("Instance type"), this);
	m_statusRight = new QLabel(tr("Assets information"), this);
	m_statusRight->setAlignment(Qt::AlignRight);
	statusBar()->addPermanentWidget(m_statusLeft, 1);
	statusBar()->addPermanentWidget(m_statusRight, 0);

	// Add "manage accounts" button, right align

	QWidget* spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	ui->mainToolBar->addWidget(spacer);

	actionManageAccounts = new QToolButton(this);
	actionManageAccounts->setToolTip(tr("Manage your Mojang or Minecraft accounts."));
	actionManageAccounts->setObjectName("actionManageAccounts");
	actionManageAccounts->setText(tr("Manage accounts"));
	actionManageAccounts->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	actionManageAccounts->setLayoutDirection(Qt::RightToLeft);

	activeAccountChanged();

	connect(actionManageAccounts, SIGNAL(clicked()), this, SLOT(on_actionManageAccounts_triggered()));
	ui->mainToolBar->addWidget(actionManageAccounts);

	// run the things that load and download other things... FIXME: this is NOT the place
	// FIXME: invisible actions in the background = NOPE.
	{
		if (!MMC->minecraftlist()->isLoaded())
		{
			m_versionLoadTask = MMC->minecraftlist()->getLoadTask();
			startTask(m_versionLoadTask);
		}
		if (!MMC->lwjgllist()->isLoaded())
		{
			MMC->lwjgllist()->loadList();
		}

		assets_downloader = new OneSixAssets();
		connect(assets_downloader, SIGNAL(indexStarted()), SLOT(assetsIndexStarted()));
		connect(assets_downloader, SIGNAL(filesStarted()), SLOT(assetsFilesStarted()));
		connect(assets_downloader, SIGNAL(filesProgress(int, int, int)),
				SLOT(assetsFilesProgress(int, int, int)));
		connect(assets_downloader, SIGNAL(failed()), SLOT(assetsFailed()));
		connect(assets_downloader, SIGNAL(finished()), SLOT(assetsFinished()));
		assets_downloader->start();
	}

	setupInstanceView();

	// removing this looks stupid
	view->setFocus();
}

MainWindow::~MainWindow()
{
	delete ui;
	delete proxymodel;
	delete drawer;
	delete assets_downloader;
}

void MainWindow::activeAccountChanged()
{
	MojangAccountPtr account = MMC->accounts()->activeAccount();

	if(account != nullptr)
	{
		actionManageAccounts->setIcon(SkinUtils::getFaceFromCache(account->username()));
	}
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
	if (obj == view)
	{
		if (ev->type() == QEvent::KeyPress)
		{
			QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
			switch (keyEvent->key())
			{
			case Qt::Key_Enter:
			case Qt::Key_Return:
				on_actionLaunchInstance_triggered();
				return true;
			case Qt::Key_Delete:
				on_actionDeleteInstance_triggered();
				return true;
			case Qt::Key_F5:
				on_actionRefresh_triggered();
				return true;
			case Qt::Key_F2:
				on_actionRenameInstance_triggered();
				return true;
			default:
				break;
			}
		}
	}
	return QMainWindow::eventFilter(obj, ev);
}

void MainWindow::onCatToggled(bool state)
{
	setCatBackground(state);
	MMC->settings()->set("TheCat", state);
}

void MainWindow::setCatBackground(bool enabled)
{
	if (enabled)
	{
		view->setStyleSheet("QListView"
							"{"
							"background-image: url(:/backgrounds/kitteh);"
							"background-attachment: fixed;"
							"background-clip: padding;"
							"background-position: top right;"
							"background-repeat: none;"
							"background-color:palette(base);"
							"}");
	}
	else
	{
		view->setStyleSheet(QString());
	}
}

void MainWindow::setupInstanceView()
{
	if (MMC->instances()->count() == 0)
	{
		return;
	}

	QMultiMap<int, QString> sorted;
	for (int i = 0; i < MMC->instances()->count(); ++i)
	{
		sorted.insertMulti(MMC->instances()->at(i)->priority(), MMC->instances()->at(i)->id());
	}
	QList<int> priorities = sorted.keys();
	std::sort(priorities.begin(), priorities.end());

	int highest = priorities.last();
	QString instanceId;
	QString lastSelectedInstance = MMC->settings()->get("SelectedInstance").toString();
	QList<QString> instancesWithHighestId;

	// sort out all instances with the highest id
	for (auto it = sorted.begin(); it != sorted.end(); ++it)
	{
		if (it.key() == highest)
		{
			instancesWithHighestId.append(it.value());
		}
	}

	if (instancesWithHighestId.contains(lastSelectedInstance))
	{
		instanceId = lastSelectedInstance;
	}
	else
	{
		// well then it's random
		instanceId = instancesWithHighestId.first();
	}

	if (!instanceId.isNull())
	{
		const QModelIndex index = MMC->instances()->getInstanceIndexById(instanceId);
		if (index.isValid())
		{
			const QModelIndex mappedIndex = proxymodel->mapFromSource(index);
			view->setCurrentIndex(mappedIndex);
		}
		else
		{
			view->setCurrentIndex(proxymodel->mapFromSource(MMC->instances()->index(0, 0)));
		}
	}
	else
	{
		view->setCurrentIndex(proxymodel->mapFromSource(MMC->instances()->index(0, 0)));
	}
}

void MainWindow::on_actionAddInstance_triggered()
{
	if (!MMC->minecraftlist()->isLoaded() && m_versionLoadTask &&
		m_versionLoadTask->isRunning())
	{
		QEventLoop waitLoop;
		waitLoop.connect(m_versionLoadTask, SIGNAL(failed(QString)), SLOT(quit()));
		waitLoop.connect(m_versionLoadTask, SIGNAL(succeeded()), SLOT(quit()));
		waitLoop.exec();
	}

	NewInstanceDialog newInstDlg(this);
	if (!newInstDlg.exec())
		return;

	BaseInstance *newInstance = NULL;

	QString instancesDir = MMC->settings()->get("InstanceDir").toString();
	QString instDirName = DirNameFromString(newInstDlg.instName(), instancesDir);
	QString instDir = PathCombine(instancesDir, instDirName);

	auto &loader = InstanceFactory::get();

	auto error = loader.createInstance(newInstance, newInstDlg.selectedVersion(), instDir);
	QString errorMsg = QString("Failed to create instance %1: ").arg(instDirName);
	switch (error)
	{
	case InstanceFactory::NoCreateError:
		newInstance->setName(newInstDlg.instName());
		newInstance->setIconKey(newInstDlg.iconKey());
		MMC->instances()->add(InstancePtr(newInstance));
		return;

	case InstanceFactory::InstExists:
	{
		errorMsg += "An instance with the given directory name already exists.";
		CustomMessageBox::selectable(this, tr("Error"), errorMsg, QMessageBox::Warning)->show();
		break;
	}

	case InstanceFactory::CantCreateDir:
	{
		errorMsg += "Failed to create the instance directory.";
		CustomMessageBox::selectable(this, tr("Error"), errorMsg, QMessageBox::Warning)->show();
		break;
	}

	default:
	{
		errorMsg += QString("Unknown instance loader error %1").arg(error);
		CustomMessageBox::selectable(this, tr("Error"), errorMsg, QMessageBox::Warning)->show();
		break;
	}
	}
}

void MainWindow::on_actionCopyInstance_triggered()
{
	if (!m_selectedInstance)
		return;

	CopyInstanceDialog copyInstDlg(m_selectedInstance, this);
	if (!copyInstDlg.exec())
		return;

	QString instancesDir = MMC->settings()->get("InstanceDir").toString();
	QString instDirName = DirNameFromString(copyInstDlg.instName(), instancesDir);
	QString instDir = PathCombine(instancesDir, instDirName);

	auto &loader = InstanceFactory::get();

	BaseInstance *newInstance = NULL;
	auto error = loader.copyInstance(newInstance, m_selectedInstance, instDir);

	QString errorMsg = QString("Failed to create instance %1: ").arg(instDirName);
	switch (error)
	{
	case InstanceFactory::NoCreateError:
		newInstance->setName(copyInstDlg.instName());
		newInstance->setIconKey(copyInstDlg.iconKey());
		MMC->instances()->add(InstancePtr(newInstance));
		return;

	case InstanceFactory::InstExists:
	{
		errorMsg += "An instance with the given directory name already exists.";
		CustomMessageBox::selectable(this, tr("Error"), errorMsg, QMessageBox::Warning)->show();
		break;
	}

	case InstanceFactory::CantCreateDir:
	{
		errorMsg += "Failed to create the instance directory.";
		CustomMessageBox::selectable(this, tr("Error"), errorMsg, QMessageBox::Warning)->show();
		break;
	}

	default:
	{
		errorMsg += QString("Unknown instance loader error %1").arg(error);
		CustomMessageBox::selectable(this, tr("Error"), errorMsg, QMessageBox::Warning)->show();
		break;
	}
	}
}

void MainWindow::on_actionChangeInstIcon_triggered()
{
	if (!m_selectedInstance)
		return;

	IconPickerDialog dlg(this);
	dlg.exec(m_selectedInstance->iconKey());
	if (dlg.result() == QDialog::Accepted)
	{
		m_selectedInstance->setIconKey(dlg.selectedIconKey);
		auto ico = MMC->icons()->getIcon(dlg.selectedIconKey);
		ui->actionChangeInstIcon->setIcon(ico);
	}
}

void MainWindow::on_actionChangeInstGroup_triggered()
{
	if (!m_selectedInstance)
		return;

	bool ok = false;
	QString name(m_selectedInstance->group());
	name = QInputDialog::getText(this, tr("Group name"), tr("Enter a new group name."),
								 QLineEdit::Normal, name, &ok);
	if (ok)
		m_selectedInstance->setGroupPost(name);
}

void MainWindow::on_actionViewInstanceFolder_triggered()
{
	QString str = MMC->settings()->get("InstanceDir").toString();
	openDirInDefaultProgram(str);
}

void MainWindow::on_actionRefresh_triggered()
{
	MMC->instances()->loadList();
}

void MainWindow::on_actionViewCentralModsFolder_triggered()
{
	openDirInDefaultProgram(MMC->settings()->get("CentralModsDir").toString(), true);
}

void MainWindow::on_actionConfig_Folder_triggered()
{
	if (m_selectedInstance)
	{
		QString str = m_selectedInstance->instanceConfigFolder();
		openDirInDefaultProgram(QDir(str).absolutePath());
	}
}

void MainWindow::on_actionCheckUpdate_triggered()
{
}

void MainWindow::on_actionSettings_triggered()
{
	SettingsDialog dialog(this);
	dialog.exec();
	// FIXME: quick HACK to make this work. improve, optimize.
	proxymodel->invalidate();
	proxymodel->sort(0);
}

void MainWindow::on_actionManageAccounts_triggered()
{
	AccountListDialog dialog(this);
	connect(&dialog, SIGNAL(activeAccountChanged()), SLOT(activeAccountChanged()));
	dialog.exec();
}

void MainWindow::on_actionReportBug_triggered()
{
	openWebPage(QUrl("http://multimc.myjetbrains.com/youtrack/dashboard#newissue=yes"));
}

void MainWindow::on_actionNews_triggered()
{
	openWebPage(QUrl("http://multimc.org/posts.html"));
}

void MainWindow::on_actionAbout_triggered()
{
	AboutDialog dialog(this);
	dialog.exec();
}

void MainWindow::on_mainToolBar_visibilityChanged(bool)
{
	// Don't allow hiding the main toolbar.
	// This is the only way I could find to prevent it... :/
	ui->mainToolBar->setVisible(true);
}

void MainWindow::on_actionDeleteInstance_triggered()
{
	if (m_selectedInstance)
	{
		auto response = CustomMessageBox::selectable(
			this, tr("CAREFUL"), tr("This is permanent! Are you sure?\nAbout to delete: ") +
									 m_selectedInstance->name(),
			QMessageBox::Question, QMessageBox::Yes | QMessageBox::No)->exec();
		if (response == QMessageBox::Yes)
		{
			m_selectedInstance->nuke();
		}
	}
}

void MainWindow::on_actionRenameInstance_triggered()
{
	if (m_selectedInstance)
	{
		bool ok = false;
		QString name(m_selectedInstance->name());
		name =
			QInputDialog::getText(this, tr("Instance name"), tr("Enter a new instance name."),
								  QLineEdit::Normal, name, &ok);

		if (name.length() > 0)
		{
			if (ok && name.length())
			{
				m_selectedInstance->setName(name);
				renameButton->setText(name);
			}
		}
	}
}

void MainWindow::on_actionViewSelectedInstFolder_triggered()
{
	if (m_selectedInstance)
	{
		QString str = m_selectedInstance->instanceRoot();
		openDirInDefaultProgram(QDir(str).absolutePath());
	}
}

void MainWindow::on_actionEditInstMods_triggered()
{
	if (m_selectedInstance)
	{
		auto dialog = m_selectedInstance->createModEditDialog(this);
		if (dialog)
			dialog->exec();
		dialog->deleteLater();
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	// Save the window state and geometry.

	MMC->settings()->set("MainWindowState", saveState().toBase64());
	MMC->settings()->set("MainWindowGeometry", saveGeometry().toBase64());

	QMainWindow::closeEvent(event);
	QApplication::exit();
}
/*
void MainWindow::on_instanceView_customContextMenuRequested(const QPoint &pos)
{
	QMenu *instContextMenu = new QMenu("Instance", this);

	// Add the actions from the toolbar to the context menu.
	instContextMenu->addActions(ui->instanceToolBar->actions());

	instContextMenu->exec(view->mapToGlobal(pos));
}
*/
void MainWindow::instanceActivated(QModelIndex index)
{
	if (!index.isValid())
		return;

	BaseInstance *inst =
		(BaseInstance *)index.data(InstanceList::InstancePointerRole).value<void *>();

	NagUtils::checkJVMArgs(inst->settings().get("JvmArgs").toString(), this);

	doLogin();
}

void MainWindow::on_actionLaunchInstance_triggered()
{
	if (m_selectedInstance)
	{
		NagUtils::checkJVMArgs(m_selectedInstance->settings().get("JvmArgs").toString(), this);
		doLogin();
	}
}

void MainWindow::doLogin(const QString &errorMsg)
{
	if (!m_selectedInstance)
		return;

	// Find an account to use.
	std::shared_ptr<MojangAccountList> accounts = MMC->accounts();
	MojangAccountPtr account = accounts->activeAccount();
	if (accounts->count() <= 0)
	{
		// Tell the user they need to log in at least one account in order to play.
		auto reply = CustomMessageBox::selectable(this, tr("No Accounts"),
			tr("In order to play Minecraft, you must have at least one Mojang or Minecraft account logged in to MultiMC."
				"Would you like to open the account manager to add an account now?"),
			QMessageBox::Information, QMessageBox::Yes | QMessageBox::No)->exec();

		if (reply == QMessageBox::Yes)
		{
			// Open the account manager.
			on_actionManageAccounts_triggered();
		}
	}
	else if (account.get() == nullptr)
	{
		// Tell the user they need to log in at least one account in order to play.
		auto reply = CustomMessageBox::selectable(this, tr("No Account Selected"),
			tr("You don't have an account selected as an active account."
				"Would you like to open the account manager to select one now?"),
			QMessageBox::Information, QMessageBox::Yes | QMessageBox::No)->exec();

		if (reply == QMessageBox::Yes)
		{
			// Open the account manager.
			on_actionManageAccounts_triggered();
		}
	}
	else
	{
		// We'll need to validate the access token to make sure the account is still logged in.
		// TODO: Do that ^
		
		prepareLaunch(m_selectedInstance, account);
	}
}

void MainWindow::prepareLaunch(BaseInstance* instance, MojangAccountPtr account)
{
	Task *updateTask = instance->doUpdate(true);
	if (!updateTask)
	{
		launchInstance(instance, account);
	}
	else
	{
		ProgressDialog tDialog(this);
		connect(updateTask, &Task::succeeded, [this, instance, account] { launchInstance(instance, account); });
		connect(updateTask, SIGNAL(failed(QString)), SLOT(onGameUpdateError(QString)));
		tDialog.exec(updateTask);
		delete updateTask;
	}

	QString playerName = account->currentProfile()->name();

	auto job = new NetJob("Player skin: " + playerName);

	auto meta = MMC->metacache()->resolveEntry("skins", playerName + ".png");
	auto action = CacheDownload::make(
		QUrl("http://skins.minecraft.net/MinecraftSkins/" + playerName + ".png"),
		meta);
	job->addNetAction(action);
	meta->stale = true;

	connect(job, SIGNAL(succeeded()), SLOT(activeAccountChanged()));
	job->start();
	auto filename = MMC->metacache()->resolveEntry("skins", "skins.json")->getFullPath();
	QFile listFile(filename);

	// Add skin mapping
	QByteArray data;
	{
		if (!listFile.open(QIODevice::ReadWrite))
		{
			QLOG_ERROR() << "Failed to open/make skins list JSON";
			return;
		}

		data = listFile.readAll();
	}

	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
	QJsonObject root = jsonDoc.object();
	QJsonObject mappings = root.value("mappings").toObject();
	QJsonArray usernames = mappings.value(account->username()).toArray();

	if (!usernames.contains(playerName))
	{
		usernames.prepend(playerName);
		mappings[account->username()] = usernames;
		root["mappings"] = mappings;
		jsonDoc.setObject(root);

		// QJson hack - shouldn't have to clear the file every time a save happens
		listFile.resize(0);
		listFile.write(jsonDoc.toJson());
	}
}

void MainWindow::launchInstance(BaseInstance *instance, MojangAccountPtr account)
{
	Q_ASSERT_X(instance != NULL, "launchInstance", "instance is NULL");
	Q_ASSERT_X(account.get() != nullptr, "launchInstance", "account is NULL");

	proc = instance->prepareForLaunch(account);
	if (!proc)
		return;

	this->hide();

	console = new ConsoleWindow(proc);
	connect(console, SIGNAL(isClosing()), this, SLOT(instanceEnded()));

	// I think this will work...
	QString username = account->username();
	QString session_id = account->accessToken();
	proc->setLogin(username, session_id);
	proc->launch();
}

void MainWindow::onGameUpdateError(QString error)
{
	CustomMessageBox::selectable(this, tr("Error updating instance"), error,
								 QMessageBox::Warning)->show();
}

void MainWindow::taskStart()
{
	// Nothing to do here yet.
}

void MainWindow::taskEnd()
{
	QObject *sender = QObject::sender();
	if (sender == m_versionLoadTask)
		m_versionLoadTask = NULL;

	sender->deleteLater();
}

void MainWindow::startTask(Task *task)
{
	connect(task, SIGNAL(started()), SLOT(taskStart()));
	connect(task, SIGNAL(succeeded()), SLOT(taskEnd()));
	connect(task, SIGNAL(failed(QString)), SLOT(taskEnd()));
	task->start();
}

// Create A Desktop Shortcut
void MainWindow::on_actionMakeDesktopShortcut_triggered()
{
	QString name("Test");
	name = QInputDialog::getText(this, tr("MultiMC Shortcut"), tr("Enter a Shortcut Name."),
								 QLineEdit::Normal, name);

	Util::createShortCut(Util::getDesktopDir(), QApplication::instance()->applicationFilePath(),
						 QStringList() << "-dl" << QDir::currentPath() << "test", name,
						 "application-x-octet-stream");

	CustomMessageBox::selectable(
		this, tr("Not useful"),
		tr("A Dummy Shortcut was created. it will not do anything productive"),
		QMessageBox::Warning)->show();
}

// BrowserDialog
void MainWindow::openWebPage(QUrl url)
{
	QDesktopServices::openUrl(url);
}

void MainWindow::on_actionChangeInstMCVersion_triggered()
{
	if (view->selectionModel()->selectedIndexes().count() < 1)
		return;

	VersionSelectDialog vselect(m_selectedInstance->versionList().get(),
								tr("Change Minecraft version"), this);
	vselect.setFilter(1, "OneSix");
	if (vselect.exec() && vselect.selectedVersion())
	{
		if (m_selectedInstance->versionIsCustom())
		{
			auto result = CustomMessageBox::selectable(
				this, tr("Are you sure?"),
				tr("This will remove any library/version customization you did previously. "
				   "This includes things like Forge install and similar."),
				QMessageBox::Warning, QMessageBox::Ok, QMessageBox::Abort)->exec();

			if (result != QMessageBox::Ok)
				return;
		}
		m_selectedInstance->setIntendedVersionId(vselect.selectedVersion()->descriptor());
	}
}

void MainWindow::on_actionChangeInstLWJGLVersion_triggered()
{
	if (!m_selectedInstance)
		return;

	LWJGLSelectDialog lselect(this);
	lselect.exec();
	if (lselect.result() == QDialog::Accepted)
	{
		LegacyInstance *linst = (LegacyInstance *)m_selectedInstance;
		linst->setLWJGLVersion(lselect.selectedVersion());
	}
}

void MainWindow::on_actionInstanceSettings_triggered()
{
	if (view->selectionModel()->selectedIndexes().count() < 1)
		return;

	InstanceSettings settings(&m_selectedInstance->settings(), this);
	settings.setWindowTitle(tr("Instance settings"));
	settings.exec();
}

void MainWindow::instanceChanged(const QModelIndex &current, const QModelIndex &previous)
{
	if (current.isValid() &&
		nullptr != (m_selectedInstance =
						(BaseInstance *)current.data(InstanceList::InstancePointerRole)
							.value<void *>()))
	{
		ui->instanceToolBar->setEnabled(true);
		QString iconKey = m_selectedInstance->iconKey();
		renameButton->setText(m_selectedInstance->name());
		ui->actionChangeInstLWJGLVersion->setEnabled(
			m_selectedInstance->menuActionEnabled("actionChangeInstLWJGLVersion"));
		ui->actionEditInstMods->setEnabled(
			m_selectedInstance->menuActionEnabled("actionEditInstMods"));
		ui->actionChangeInstMCVersion->setEnabled(
			m_selectedInstance->menuActionEnabled("actionChangeInstMCVersion"));
		m_statusLeft->setText(m_selectedInstance->getStatusbarDescription());
		auto ico = MMC->icons()->getIcon(iconKey);
		ui->actionChangeInstIcon->setIcon(ico);

		MMC->settings()->set("SelectedInstance", m_selectedInstance->id());
	}
	else
	{
		selectionBad();

		MMC->settings()->set("SelectedInstance", QString());
	}
}

void MainWindow::selectionBad()
{
	m_selectedInstance = nullptr;
	QString iconKey = "infinity";
	statusBar()->clearMessage();
	ui->instanceToolBar->setEnabled(false);
	renameButton->setText(tr("Rename Instance"));
	auto ico = MMC->icons()->getIcon(iconKey);
	ui->actionChangeInstIcon->setIcon(ico);
}

void MainWindow::on_actionEditInstNotes_triggered()
{
	if (!m_selectedInstance)
		return;
	LegacyInstance *linst = (LegacyInstance *)m_selectedInstance;

	EditNotesDialog noteedit(linst->notes(), linst->name(), this);
	noteedit.exec();
	if (noteedit.result() == QDialog::Accepted)
	{

		linst->setNotes(noteedit.getText());
	}
}

void MainWindow::instanceEnded()
{
	this->show();
}

void MainWindow::checkSetDefaultJava()
{
	bool askForJava = false;
	{
		QString currentHostName = QHostInfo::localHostName();
		QString oldHostName = MMC->settings()->get("LastHostname").toString();
		if (currentHostName != oldHostName)
		{
			MMC->settings()->set("LastHostname", currentHostName);
			askForJava = true;
		}
	}

	{
		QString currentJavaPath = MMC->settings()->get("JavaPath").toString();
		if (currentJavaPath.isEmpty())
		{
			askForJava = true;
		}
	}

	if (askForJava)
	{
		QLOG_DEBUG() << "Java path needs resetting, showing Java selection dialog...";

		JavaVersionPtr java;

		VersionSelectDialog vselect(MMC->javalist().get(), tr("Select a Java version"), this,
									false);
		vselect.setResizeOn(2);
		vselect.exec();

		if (vselect.selectedVersion())
			java = std::dynamic_pointer_cast<JavaVersion>(vselect.selectedVersion());
		else
		{
			CustomMessageBox::selectable(
				this, tr("Invalid version selected"),
				tr("You didn't select a valid Java version, so MultiMC will "
				   "select the default. "
				   "You can change this in the settings dialog."),
				QMessageBox::Warning)->show();

			JavaUtils ju;
			java = ju.GetDefaultJava();
		}
		if (java)
			MMC->settings()->set("JavaPath", java->path);
		else
			MMC->settings()->set("JavaPath", QString("java"));
	}
}

void MainWindow::assetsIndexStarted()
{
	m_statusRight->setText(tr("Checking assets..."));
}

void MainWindow::assetsFilesStarted()
{
	m_statusRight->setText(tr("Downloading assets..."));
}

void MainWindow::assetsFilesProgress(int succeeded, int failed, int total)
{
	QString status = tr("Downloading assets: %1 / %2").arg(succeeded + failed).arg(total);
	if (failed > 0)
		status += tr(" (%1 failed)").arg(failed);
	status += tr("...");
	m_statusRight->setText(status);
}

void MainWindow::assetsFailed()
{
	m_statusRight->setText(tr("Failed to update assets."));
}

void MainWindow::assetsFinished()
{
	m_statusRight->setText(tr("Assets up to date."));
}
