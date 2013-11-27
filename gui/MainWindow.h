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

#pragma once

#include <QMainWindow>
#include <QProcess>

#include "logic/lists/InstanceList.h"
#include "logic/net/LoginTask.h"
#include "logic/BaseInstance.h"

#include "logic/auth/MojangAccount.h"

class QToolButton;
class LabeledToolButton;
class QLabel;
class InstanceProxyModel;
class KCategorizedView;
class KCategoryDrawer;
class MinecraftProcess;
class ConsoleWindow;
class OneSixAssets;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	void closeEvent(QCloseEvent *event);

	// Browser Dialog
	void openWebPage(QUrl url);

	void checkSetDefaultJava();

private
slots:
	void onCatToggled(bool);

	void on_actionAbout_triggered();

	void on_actionAddInstance_triggered();

	void on_actionCopyInstance_triggered();

	void on_actionChangeInstGroup_triggered();

	void on_actionChangeInstIcon_triggered();

	void on_actionViewInstanceFolder_triggered();

	void on_actionConfig_Folder_triggered();

	void on_actionViewSelectedInstFolder_triggered();

	void on_actionRefresh_triggered();

	void on_actionViewCentralModsFolder_triggered();

	void on_actionCheckUpdate_triggered();

	void on_actionSettings_triggered();

	void on_actionManageAccounts_triggered();

	void on_actionReportBug_triggered();

	void on_actionNews_triggered();

	void on_mainToolBar_visibilityChanged(bool);

	//	void on_instanceView_customContextMenuRequested(const QPoint &pos);

	void on_actionLaunchInstance_triggered();

	void on_actionDeleteInstance_triggered();

	void on_actionRenameInstance_triggered();

	void on_actionMakeDesktopShortcut_triggered();

	void on_actionChangeInstMCVersion_triggered();

	void on_actionEditInstMods_triggered();

	void on_actionEditInstNotes_triggered();

	void doLogin(const QString &errorMsg = "");

	/*!
	 * Launches the given instance with the given account.
	 * This function assumes that the given account has a valid, usable access token.
	 */
	void launchInstance(BaseInstance* instance, MojangAccountPtr account);

	/*!
	 * Prepares the given instance for launch with the given account.
	 */
	void prepareLaunch(BaseInstance* instance, MojangAccountPtr account);

	void onGameUpdateError(QString error);

	void taskStart();
	void taskEnd();

	void on_actionChangeInstLWJGLVersion_triggered();

	void instanceEnded();

	void on_actionInstanceSettings_triggered();

	void assetsIndexStarted();
	void assetsFilesStarted();
	void assetsFilesProgress(int, int, int);
	void assetsFailed();
	void assetsFinished();

public
slots:
	void instanceActivated(QModelIndex);

	void instanceChanged(const QModelIndex &current, const QModelIndex &previous);

	void selectionBad();

	void startTask(Task *task);

	void activeAccountChanged();

protected:
	bool eventFilter(QObject *obj, QEvent *ev);
	void setCatBackground(bool enabled);

	void setupInstanceView();

private:
	Ui::MainWindow *ui;
	KCategoryDrawer *drawer;
	KCategorizedView *view;
	InstanceProxyModel *proxymodel;
	MinecraftProcess *proc;
	ConsoleWindow *console;
	OneSixAssets *assets_downloader;
	LabeledToolButton *renameButton;
	QToolButton *changeIconButton;

	BaseInstance *m_selectedInstance;

	Task *m_versionLoadTask;

	QLabel *m_statusLeft;
	QLabel *m_statusRight;

	QToolButton *actionManageAccounts;
};
