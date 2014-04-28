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
#include "logic/quickmod/QuickModDependencyResolver.h"
#include "logic/BaseInstance.h"
#include "logic/net/ByteArrayDownload.h"
#include "logic/net/NetJob.h"
#include "MultiMC.h"
#include "depends/settings/settingsobject.h"
#include "depends/quazip/JlCompress.h"
#include "logic/quickmod/QuickModInstaller.h"

Q_DECLARE_METATYPE(QTreeWidgetItem *)

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

QuickModInstallDialog::QuickModInstallDialog(InstancePtr instance, QWidget *parent)
	: QDialog(parent), ui(new Ui::QuickModInstallDialog), m_installer(new QuickModInstaller(this, this)), m_instance(instance)
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

QTreeWidgetItem* QuickModInstallDialog::addProgressListEntry(QuickModVersionPtr version, const QString& url, const QString& progressMsg)
{
	auto item = new QTreeWidgetItem(ui->progressList);
	item->setText(0, version->mod->name());
	item->setIcon(0, version->mod->icon());
	item->setText(1, version->name());
	item->setText(2, url);

	if (progressMsg.isEmpty())
		item->setData(3, ExtraRoles::IgnoreRole, false);
	else
	{
		item->setData(3, ExtraRoles::IgnoreRole, true);
		item->setText(3, progressMsg);
		item->setData(3, Qt::ForegroundRole, QColor(Qt::darkGreen));
	}

	return item;
}

void QuickModInstallDialog::setWebViewShown(bool shown)
{
	if (shown) ui->downloadSplitter->setSizes(QList<int>({100, 500}));
	else       ui->downloadSplitter->setSizes(QList<int>({100, 0}));
}


int QuickModInstallDialog::exec()
{
	showMaximized();

	if (!downloadDeps())
	{
		QMessageBox::critical(this, tr("Error"), tr("Failed to download QuickMod dependencies."));
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
		//displayMessage(tr("No mods need to be downloaded."));
		return QDialog::exec();
	}

	runDirectDownloads();

	// Initialize the web mods progress bar.
	ui->webModsProgressBar->setMaximum(m_modVersions.size());
	ui->webModsProgressBar->setValue(0);
	downloadNextMod();

	return QDialog::exec();
}


bool QuickModInstallDialog::downloadDeps()
{
	// download all dependency files so they are ready for the dependency resolution
	ProgressDialog dialog(this);
	auto task = new QuickModDependencyDownloadTask(m_initialMods, this);
	return dialog.exec(task) != QDialog::Rejected; // Is this really the best way to check for failure?
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

void QuickModInstallDialog::processVersionList()
{
	foreach(QuickModVersionPtr version, m_modVersions)
	{
		// TODO: Any installed version should prevent another version from being installed
		// TODO: Notify the user when this happens.
		if (MMC->quickmodslist()->isModMarkedAsInstalled(version->mod, version, m_instance))
		{
			QLOG_INFO() << version->mod->uid() << " is already installed";
			m_modVersions.removeAll(version);
		}

		if (MMC->quickmodslist()->isModMarkedAsExists(version->mod, version))
		{
			QLOG_INFO() << version->mod->uid() << " exists already. Only installing.";
			install(version);
			addProgressListEntry(version, "N/A", tr("Success: Installed from cache."));
			m_modVersions.removeAll(version);
		}

		if (!version->url.isValid())
		{
			m_modVersions.removeAll(version);
		}
	}
}

void QuickModInstallDialog::runDirectDownloads()
{
	// do all of the direct download mods
	QMutableListIterator<QuickModVersionPtr> it(m_modVersions);
	while (it.hasNext())
	{
		QuickModVersionPtr version = it.next();
		if (version->downloadType == QuickModVersion::Direct)
		{
			QLOG_DEBUG() << "Direct download used for " << version->url.toString();
			processReply(MMC->qnam()->get(QNetworkRequest(version->url)), version);
			it.remove();
		}
	}
}

void QuickModInstallDialog::setInitialMods(const QList<QuickModPtr> mods)
{
	m_initialMods = mods;
}

void QuickModInstallDialog::downloadNextMod()
{
	if (checkIsDone()) return;
	if (m_modVersions.isEmpty()) return;

	auto version = m_modVersions.takeFirst();
	ui->webModsProgressBar->setValue(ui->webModsProgressBar->value() + 1);
	runWebDownload(version);
}

void QuickModInstallDialog::runWebDownload(QuickModVersionPtr version)
{
	QLOG_INFO() << "Downloading " << version->name() << "(" << version->url.toString() << ")";

	auto navigator = new WebDownloadNavigator(this);
	navigator->setProperty("version", QVariant::fromValue(version));
	connect(navigator, &WebDownloadNavigator::caughtUrl, this, &QuickModInstallDialog::urlCaught);
	navigator->load(version->url);
	ui->webTabView->addTab(navigator, (QString("%1 %2").arg(version->mod->name(), version->name())));

	setWebViewShown(true);
}

bool QuickModInstallDialog::checkIsDone()
{
	if (m_downloadingUrls.isEmpty() && ui->webTabView->count() == 0 && m_modVersions.isEmpty())
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

bool QuickModInstallDialog::install(QuickModVersionPtr version, QTreeWidgetItem *item)
{
	try
	{
		m_installer->install(version, m_instance);
	}
	catch (MMCError &e)
	{
		if (item)
		{
			item->setText(3, e.cause());
			item->setData(3, Qt::ForegroundRole, QColor(Qt::red));
		}
		return false;
	}
	return true;
}

void QuickModInstallDialog::urlCaught(QNetworkReply *reply)
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

	auto navigator = qobject_cast<WebDownloadNavigator *>(sender());

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

	auto item = addProgressListEntry(version, reply->url().toString(QUrl::PrettyDecoded));
	reply->setProperty("item", QVariant::fromValue(item));
	reply->setProperty("version", QVariant::fromValue(version));

	connect(reply, &QNetworkReply::downloadProgress, this,
			&QuickModInstallDialog::downloadProgress);
	connect(reply, &QNetworkReply::finished, this, &QuickModInstallDialog::downloadCompleted);
}

void QuickModInstallDialog::downloadProgress(const qint64 current, const qint64 max)
{
	auto reply = qobject_cast<QNetworkReply *>(sender());
	auto item = reply->property("item").value<QTreeWidgetItem *>();
	item->setData(3, ExtraRoles::ProgressRole, current);
	item->setData(3, ExtraRoles::TotalRole, max);
	ui->progressList->update(
		ui->progressList->model()->index(ui->progressList->indexOfTopLevelItem(item), 3));
}

void QuickModInstallDialog::downloadCompleted()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
	m_downloadingUrls.removeAll(reply->url());
	auto item = reply->property("item").value<QTreeWidgetItem *>();
	item->setData(3, ExtraRoles::IgnoreRole, true);
	item->setText(3, tr("Installing..."));
	auto version = reply->property("version").value<QuickModVersionPtr>();

	try
	{
		m_installer->handleDownload(version, reply->readAll(), reply->url());
	}
	catch (MMCError &e)
	{
		item->setText(3, e.cause());
		item->setData(3, Qt::ForegroundRole, QColor(Qt::red));
	}
	reply->close();
	reply->deleteLater();

	if (install(version, item))
	{
		item->setText(3, tr("Success: Downloaded and installed successfully."));
		item->setData(3, Qt::ForegroundRole, QColor(Qt::darkGreen));
	}

	checkIsDone();
}

#include "QuickModInstallDialog.moc"
