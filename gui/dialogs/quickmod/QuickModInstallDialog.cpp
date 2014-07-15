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

#include "QuickModInstallDialog.h"
#include "ui_QuickModInstallDialog.h"

#include <QTreeWidgetItem>
#include <QNetworkReply>
#include <QStyledItemDelegate>
#include <QProgressBar>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QtMath>
#include <QCryptographicHash>

#include "gui/widgets/WebDownloadNavigator.h"
#include "gui/dialogs/ProgressDialog.h"
#include "gui/dialogs/VersionSelectDialog.h"
#include "logic/quickmod/QuickModsList.h"
#include "logic/quickmod/QuickMod.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/tasks/QuickModDependencyDownloadTask.h"
#include "logic/quickmod/tasks/QuickModMavenFindTask.h"
#include "logic/quickmod/QuickModDependencyResolver.h"
#include "logic/BaseInstance.h"
#include "logic/net/ByteArrayDownload.h"
#include "logic/net/NetJob.h"
#include "MultiMC.h"
#include "logic/settings/SettingsObject.h"
#include "depends/quazip/JlCompress.h"
#include "logic/quickmod/QuickModInstaller.h"

Q_DECLARE_METATYPE(QTreeWidgetItem *)

// {{{ Data structures and classes

// TODO load parallel versions in parallel (makes sense, right?)

struct ExtraRoles
{
	enum
	{
		ProgressRole = Qt::UserRole,
		TotalRole,
		IgnoreRole
	};
};

class ProgressItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	ProgressItemDelegate(QObject *parent = 0) : QStyledItemDelegate(parent)
	{
	}

	void paint(QPainter *painter, const QStyleOptionViewItem &opt,
			   const QModelIndex &index) const
	{
		QStyledItemDelegate::paint(painter, opt, index);

		if (!index.data(ExtraRoles::IgnoreRole).toBool())
		{
			qlonglong progress = index.data(ExtraRoles::ProgressRole).toLongLong();
			qlonglong total = index.data(ExtraRoles::TotalRole).toLongLong();

			int percent = total == 0 ? 0 : qFloor(progress * 100 / total);

			QStyleOptionProgressBar style;
			style.rect = opt.rect;
			style.minimum = 0;
			style.maximum = total;
			style.progress = progress;
			style.text = QString("%1 %").arg(percent);
			style.textVisible = true;

			QApplication::style()->drawControl(QStyle::CE_ProgressBar, &style, painter);
		}
	}
};

// }}}

// {{{ Initialization and updating

QuickModInstallDialog::QuickModInstallDialog(InstancePtr instance, QWidget *parent)
	: QDialog(parent), ui(new Ui::QuickModInstallDialog),
	  m_installer(new QuickModInstaller(this, this)), m_instance(instance)
{
	ui->setupUi(this);
	setWindowModality(Qt::WindowModal);

	setWebViewShown(false);

	ui->progressList->setItemDelegateForColumn(3, new ProgressItemDelegate(this));

	ui->webModsProgressBar->setMaximum(0);
	ui->webModsProgressBar->setValue(0);

	// Set the URL column's width.
	ui->progressList->setColumnWidth(2, 420);
}

QuickModInstallDialog::~QuickModInstallDialog()
{
	delete ui;
}

// }}}

// {{{ Utility functions

// {{{ Progress list
void QuickModInstallDialog::addProgressListEntry(QuickModVersionPtr version, const QString &url)
{
	auto item = new QTreeWidgetItem(ui->progressList);
	m_progressEntries.insert(version.get(), item);
	item->setText(0, version->mod->name());
	item->setIcon(0, version->mod->icon());
	item->setText(1, version->name());
	item->setText(2, url);
	item->setData(3, ExtraRoles::IgnoreRole, true);
	item->setText(3, tr("Download waiting."));
}

void QuickModInstallDialog::setProgressListUrl(QuickModVersionPtr version, const QString &url)
{
	auto item = itemForVersion(version);
	item->setText(2, url);
}

void QuickModInstallDialog::setProgressListMsg(QuickModVersionPtr version, const QString &msg,
											   const QColor &color)
{
	auto item = itemForVersion(version);
	item->setData(3, ExtraRoles::IgnoreRole, true);
	item->setText(3, msg);
	item->setData(3, Qt::ForegroundRole, color);
}

void QuickModInstallDialog::setVersionProgress(QuickModVersionPtr version, qint64 current,
											   qint64 max)
{
	auto item = itemForVersion(version);
	item->setData(3, ExtraRoles::IgnoreRole, false);
	item->setData(3, ExtraRoles::ProgressRole, current);
	item->setData(3, ExtraRoles::TotalRole, max);
}

void QuickModInstallDialog::setShowProgressBar(QuickModVersionPtr version, bool show)
{
	auto item = itemForVersion(version);
	item->setData(3, ExtraRoles::IgnoreRole, !show);
}

QTreeWidgetItem *QuickModInstallDialog::itemForVersion(QuickModVersionPtr version) const
{
	return m_progressEntries.value(version.get());
}
// }}}

// {{{ Other
bool QuickModInstallDialog::checkIsDone()
{
	if (m_downloadingUrls.isEmpty() && m_mavenFindTasks.isEmpty() && ui->webTabView->count() == 0 && m_modVersions.isEmpty())
	{
		ui->finishButton->setEnabled(true);
		return true;
	}
	else
	{
		ui->finishButton->setDisabled(true);
		return false;
	}
}

void QuickModInstallDialog::setWebViewShown(bool shown)
{
	if (shown)
		ui->downloadSplitter->setSizes(QList<int>({100, 500}));
	else
		ui->downloadSplitter->setSizes(QList<int>({100, 0}));
}

void QuickModInstallDialog::setInitialMods(const QList<QuickModUid> mods)
{
	m_initialMods = mods;
}
// }}}

// }}}

// {{{ Installer execution

// {{{ Main function

int QuickModInstallDialog::exec()
{
	showMaximized();

	if (!downloadDeps())
	{
		QMessageBox::critical(this, tr("Error"),
							  tr("Failed to download QuickMod dependencies."));
		return QDialog::Rejected;
	}
	if (resolveDeps())
	{
		// If we resolved dependencies successfully, switch to the download tab.
		ui->tabWidget->setCurrentIndex(1);
	}
	else
	{
		// Otherwise, stop and display the dependency resolution list.
		ui->tabWidget->setCurrentIndex(0);
		QDialog::exec();
		return QDialog::Rejected;
	}

	processVersionList();

	if (checkIsDone())
	{
		// displayMessage(tr("No mods need to be downloaded."));
		return QDialog::exec();
	}

	selectDownloadUrls();

	runDirectDownloads();

	runMavenDownloads();

	// Initialize the web mods progress bar.
	ui->webModsProgressBar->setMaximum(m_modVersions.size());
	ui->webModsProgressBar->setValue(0);
	downloadNextMod();

	return QDialog::exec();
}

// }}}

// {{{ Sub-functions

// {{{ Dependency resolution and downloading
bool QuickModInstallDialog::downloadDeps()
{
	// download all dependency files so they are ready for the dependency resolution
	ProgressDialog dialog(this);
	auto task = new QuickModDependencyDownloadTask(m_initialMods, this);
	return dialog.exec(task) !=
		   QDialog::Rejected; // Is this really the best way to check for failure?
}

bool QuickModInstallDialog::resolveDeps()
{
	// Resolve dependencies
	bool error = false;
	QuickModDependencyResolver resolver(m_instance, this);

	// Error handler.
	connect(&resolver, &QuickModDependencyResolver::error, [this, &error](const QString &msg)
	{
		error = true;
		QListWidgetItem *item = new QListWidgetItem(msg);
		item->setTextColor(Qt::red);
		ui->dependencyListWidget->addItem(item);
	});

	// Warning handler.
	connect(&resolver, &QuickModDependencyResolver::warning, [this](const QString &msg)
	{
		QListWidgetItem *item = new QListWidgetItem(msg);
		item->setTextColor(Qt::darkYellow);
		ui->dependencyListWidget->addItem(item);
	});

	// Success handler.
	connect(&resolver, &QuickModDependencyResolver::success, [this](const QString &msg)
	{
		QListWidgetItem *item = new QListWidgetItem(msg);
		item->setTextColor(Qt::darkGreen);
		ui->dependencyListWidget->addItem(item);
	});

	m_modVersions = m_resolvedVersions = resolver.resolve(m_initialMods);
	return !error;
}
// }}}

// {{{ Process version list
void QuickModInstallDialog::processVersionList()
{
	QMutableListIterator<QuickModVersionPtr> it(m_modVersions);
	while (it.hasNext())
	{
		QuickModVersionPtr version = it.next();

		// TODO: Any installed version should prevent another version from being installed
		// TODO: Notify the user when this happens.
		if (MMC->quickmodslist()->isModMarkedAsInstalled(version->mod, version, m_instance))
		{
			QLOG_INFO() << version->mod->uid() << " is already installed";
			it.remove();
			continue;
		}

		// Add a progress list entry for each download
		addProgressListEntry(version);

		if (MMC->quickmodslist()->isModMarkedAsExists(version->mod, version))
		{
			QLOG_INFO() << version->mod->uid() << " exists already. Only installing.";
			install(version);
			setProgressListMsg(version, tr("Success: Installed from cache."),
							   QColor(Qt::darkGreen));
			it.remove();
		}

		if (version->downloads.isEmpty())
		{
			it.remove();
			setProgressListMsg(version, tr("Error: No download URLs"), QColor(Qt::red));
		}
	}
}
// }}}

// {{{ Select download urls
void QuickModInstallDialog::selectDownloadUrls()
{
	QMutableListIterator<QuickModVersionPtr> it(m_modVersions);
	while (it.hasNext())
	{
		QuickModVersionPtr version = it.next();
		setProgressListMsg(version, tr("Selecting URL..."));

		// TODO ask the user + settings

		m_selectedDownloadUrls.insert(version, version->downloads.first());

		clearProgressListMsg(version);
	}
}
// }}}

// {{{ Run direct downloads
void QuickModInstallDialog::runDirectDownloads()
{
	// do all of the direct download mods
	QMutableListIterator<QuickModVersionPtr> it(m_modVersions);
	while (it.hasNext())
	{
		QuickModVersionPtr version = it.next();
		QuickModDownload download = m_selectedDownloadUrls[version];
		if (download.type == QuickModDownload::Direct)
		{
			QLOG_DEBUG() << "Direct download used for" << download.url;
			processReply(MMC->qnam()->get(QNetworkRequest(QUrl(download.url))), version);
			it.remove();
		}
	}
}

void QuickModInstallDialog::runMavenDownloads()
{
	// do all of the maven downloads
	QMutableListIterator<QuickModVersionPtr> it(m_modVersions);
	while (it.hasNext())
	{
		QuickModVersionPtr version = it.next();
		QuickModDownload download = m_selectedDownloadUrls[version];
		if (download.type == QuickModDownload::Maven)
		{
			QLOG_DEBUG() << "Checking Maven repositories for " << download.url;
			setProgressListMsg(version, tr("Checking Maven repositories"));
			QuickModMavenFindTask *task = new QuickModMavenFindTask(version->mod->mavenRepos(), download.url);
			connect(task, &QuickModMavenFindTask::failed, [this, version, download]()
			{
				QLOG_ERROR() << "Couldn't find a maven download for" << download.url;
				setProgressListMsg(version, tr("Couldn't find a Maven repository"), Qt::red);
			});
			connect(task, &QuickModMavenFindTask::succeeded, [this, version, download, task]()
			{
				QLOG_DEBUG() << "Found Maven download for" << download.url << "at" << task->url().toString();
				processReply(MMC->qnam()->get(QNetworkRequest(task->url())), version);
			});
			m_mavenFindTasks.append(task);
			task->start();
			it.remove();
		}
	}
}
// }}}

// {{{ Webpage mod downloads
void QuickModInstallDialog::downloadNextMod()
{
	if (checkIsDone())
		return;
	if (m_modVersions.isEmpty())
		return;

	auto version = m_modVersions.takeFirst();
	runWebDownload(version);
}

void QuickModInstallDialog::runWebDownload(QuickModVersionPtr version)
{
	const QUrl url = QUrl(m_selectedDownloadUrls[version].url);
	QLOG_INFO() << "Downloading " << version->name() << "(" << url.toString() << ")";

	setProgressListMsg(version, tr("Waiting on web page."), QColor(Qt::blue));

	auto navigator = new WebDownloadNavigator(this);
	navigator->load(url);
	ui->webTabView->addTab(navigator,
						   (QString("%1 %2").arg(version->mod->name(), version->name())));

	navigator->setProperty("version", QVariant::fromValue(version));
	connect(navigator, &WebDownloadNavigator::caughtUrl, [this, navigator](QNetworkReply *reply)
	{
		ui->webModsProgressBar->setValue(ui->webModsProgressBar->value() + 1);
		urlCaught(reply, navigator);
	});

	setWebViewShown(true);
}
// }}}

// }}}

// {{{ Other stuff

bool QuickModInstallDialog::install(QuickModVersionPtr version)
{
	try
	{
		m_installer->install(version, m_instance);
	}
	catch (MMCError *e)
	{
		setProgressListMsg(version, e->cause(), QColor(Qt::red));
		return false;
	}
	return true;
}

// }}}

// }}}

// {{{ Event handlers

void QuickModInstallDialog::urlCaught(QNetworkReply *reply, WebDownloadNavigator *navigator)
{
	// TODO: Do more to verify that the correct file is being downloaded.
	if (reply->url().path().endsWith(".exe"))
	{
		// because bad things
		QLOG_WARN() << "Caught .exe from" << reply->url().toString();
		return;
	}
	QLOG_INFO() << "Caught " << reply->url().toString();
	downloadNextMod();

	if (!navigator)
	{
		navigator = qobject_cast<WebDownloadNavigator *>(sender());
	}

	processReply(reply, navigator->property("version").value<QuickModVersionPtr>());

	ui->webTabView->removeTab(ui->webTabView->indexOf(navigator));
	navigator->deleteLater();
	if (ui->webTabView->count() == 0)
	{
		setWebViewShown(false);
	}
}

void QuickModInstallDialog::processReply(QNetworkReply *reply, QuickModVersionPtr version)
{
	m_downloadingUrls.append(reply->url());

	setProgressListUrl(version, reply->url().toString(QUrl::PrettyDecoded));
	reply->setProperty("version", QVariant::fromValue(version));

	connect(reply, &QNetworkReply::downloadProgress, this,
			&QuickModInstallDialog::downloadProgress);
	connect(reply, &QNetworkReply::finished, this, &QuickModInstallDialog::downloadCompleted);
}

void QuickModInstallDialog::downloadProgress(const qint64 current, const qint64 max)
{
	auto reply = qobject_cast<QNetworkReply *>(sender());
	auto version = reply->property("version").value<QuickModVersionPtr>();
	setVersionProgress(version, current, max);
	// ui->progressList->update(ui->progressList->model()->index(ui->progressList->indexOfTopLevelItem(item),
	// 3));
}

void QuickModInstallDialog::downloadCompleted()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
	m_downloadingUrls.removeAll(reply->url());
	auto version = reply->property("version").value<QuickModVersionPtr>();

	// redirection
	const QVariant location = reply->header(QNetworkRequest::LocationHeader);
	if (location.isValid())
	{
		const QUrl locationUrl = QUrl(location.toString());
		processReply(MMC->qnam()->get(QNetworkRequest(locationUrl)), version);
		m_downloadingUrls.append(locationUrl);
		return;
	}

	setProgressListMsg(version, tr("Installing"));

	bool fail = false;
	try
	{
		m_installer->handleDownload(version, reply->readAll(), reply->url());
	}
	catch (MMCError *e)
	{
		setProgressListMsg(version, e->cause(), QColor(Qt::red));
		fail = true;
	}
	reply->close();
	reply->deleteLater();

	if (!fail && install(version))
	{
		setProgressListMsg(version, tr("Success: Downloaded and installed successfully."),
						   QColor(Qt::darkGreen));
	}

	checkIsDone();
}

// }}}

#include "QuickModInstallDialog.moc"
