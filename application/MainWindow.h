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

#pragma once

#include <QMainWindow>
#include <QProcess>
#include <QTimer>

#include "BaseInstance.h"
#include "minecraft/auth/MojangAuthSession.h"
#include "net/NetJob.h"
#include "updater/GoUpdate.h"

class NewsChecker;
class NotificationChecker;
class QToolButton;
class InstanceProxyModel;
class LabeledToolButton;
class QLabel;
class Process;
class ConsoleWindow;
class BaseProfilerFactory;
class GenericPageProvider;

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
	void checkInstancePathForProblems();

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

	void on_actionInstanceSettings_triggered();

	void on_actionManageAccounts_triggered();

	void on_actionReportBug_triggered();

	void on_actionPatreon_triggered();

	void on_actionMoreNews_triggered();

	void newsButtonClicked();

	void on_mainToolBar_visibilityChanged(bool);

	//	void on_instanceView_customContextMenuRequested(const QPoint &pos);

	void on_actionLaunchInstance_triggered();

	void on_actionLaunchInstanceOffline_triggered();

	void on_actionDeleteInstance_triggered();

	void on_actionExportInstance_triggered();

	void on_actionRenameInstance_triggered();

	void on_actionEditInstance_triggered();

	void on_actionEditInstNotes_triggered();

	void on_actionScreenshots_triggered();

	/*!
	 * Launches the currently selected instance with the default account.
	 * If no default account is selected, prompts the user to pick an account.
	 */
	void doLaunch(bool online = true, BaseProfilerFactory *profiler = 0);

	/*!
	 * Launches the given instance with the given account.
	 * This function assumes that the given account has a valid, usable access token.
	 */
	void launchInstance(InstancePtr instance, SessionPtr session, BaseProfilerFactory *profiler = 0);

	/*!
	 * Prepares the given instance for launch with the given account.
	 */
	void updateInstance(InstancePtr instance, SessionPtr account, BaseProfilerFactory *profiler = 0);

	void onGameUpdateError(QString error);

	// called when an icon is changed in the icon model.
	void iconUpdated(QString);

	void showInstanceContextMenu(const QPoint &);

	void updateToolsMenu();

public
slots:
	void instanceActivated(QModelIndex);
	void instanceChanged(const QModelIndex &current, const QModelIndex &previous);

	void instanceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

	void selectionBad();

	void updateAvailable(GoUpdate::Status status);
	void updateNotAvailable();

	void notificationsChanged();

	void latestAccountChanged();
	void makeAccountGlobalDefault();
	void repopulateAccountsMenu();

	void updateNewsLabel();

	/*!
	 * Runs the DownloadTask and installs updates.
	 */
	void downloadUpdates(GoUpdate::Status status, bool installOnExit = false);

protected:
	bool eventFilter(QObject *obj, QEvent *ev);
	void setCatBackground(bool enabled);
	void updateInstanceToolIcon(QString new_icon);

	void setSelectedInstanceById(const QString &id);

	void instanceFromVersion(QString instName, QString instGroup, QString instIcon, BaseVersionPtr version);
	void instanceFromZipPack(QString instName, QString instGroup, QString instIcon, QUrl url);
	void finalizeInstance(InstancePtr inst);

private:
	Ui::MainWindow *ui;
	class GroupView *view;
	InstanceProxyModel *proxymodel;
    NetJobPtr skin_download_job;
	Process *proc;
	ConsoleWindow *console;
	LabeledToolButton *renameButton;
	QToolButton *changeIconButton;
	QToolButton *newsLabel;

	std::shared_ptr<GenericPageProvider> m_globalSettingsProvider;
	std::shared_ptr<NewsChecker> m_newsChecker;
	std::shared_ptr<NotificationChecker> m_notificationChecker;

	InstancePtr m_selectedInstance;
	QString m_currentInstIcon;

	Task *m_versionLoadTask;

	QLabel *m_statusLeft;
	class ServerStatus *m_statusRight;

	QMenu *accountMenu;
	QToolButton *accountMenuButton;
	QAction *manageAccountsAction;
};
