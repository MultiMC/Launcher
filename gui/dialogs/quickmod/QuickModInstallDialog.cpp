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

	ui->progressList->setItemDelegateForColumn(3, new ProgressItemDelegate(this));

	connect(ui->nextButton, &QPushButton::clicked, [this]()
	{
		if (ui->web->currentIndex() + 1 < ui->web->count())
		{
			ui->web->setCurrentIndex(ui->web->currentIndex() + 1);
		}
		else
		{
			ui->web->setCurrentIndex(0);
		}
	});
	connect(ui->previousButton, &QPushButton::clicked, [this]()
	{
		if (ui->web->currentIndex() - 1 >= 0)
		{
			ui->web->setCurrentIndex(ui->web->currentIndex() - 1);
		}
		else
		{
			ui->web->setCurrentIndex(ui->web->count() - 1);
		}
	});
}

QuickModInstallDialog::~QuickModInstallDialog()
{
	delete ui;
}

int QuickModInstallDialog::exec()
{
	// download all dependency files so they are ready for the dependency resolution
	{
		ProgressDialog dialog(this);
		if (dialog.exec(new QuickModDependencyDownloadTask(m_initialMods, this)) ==
			QDialog::Rejected)
		{
			return QDialog::Rejected;
		}
	}

	showMaximized();

	// resolve dependencies
	{
		bool error = false;
		QuickModDependencyResolver resolver(m_instance, this);
		connect(&resolver, &QuickModDependencyResolver::error,
				[this, &error](const QString &msg)
		{
			error = true;
			QListWidgetItem *item = new QListWidgetItem(msg);
			item->setTextColor(Qt::red);
			ui->dependencyListWidget->addItem(item);
		});
		connect(&resolver, &QuickModDependencyResolver::warning, [this](const QString &msg)
		{
			QListWidgetItem *item = new QListWidgetItem(msg);
			item->setTextColor(Qt::darkYellow);
			ui->dependencyListWidget->addItem(item);
		});
		connect(&resolver, &QuickModDependencyResolver::success, [this](const QString &msg)
		{
			QListWidgetItem *item = new QListWidgetItem(msg);
			item->setTextColor(Qt::darkGreen);
			ui->dependencyListWidget->addItem(item);
		});
		m_modVersions = m_resolvedVersions = resolver.resolve(m_initialMods);
		if (!error)
		{
			ui->tabWidget->setCurrentIndex(1);
		}
		else
		{
			return QDialog::exec();
		}
	}

	foreach(QuickModVersionPtr version, m_modVersions)
	{
		// TODO any installed version should prevent another version from being installed
		if (MMC->quickmodslist()->isModMarkedAsInstalled(version->mod, version, m_instance.get()))
		{
			QLOG_INFO() << version->mod->uid() << " is already installed";
			m_modVersions.removeAll(version);
		}

		if (MMC->quickmodslist()->isModMarkedAsExists(version->mod, version))
		{
			QLOG_INFO() << version->mod->uid() << " exists already. Only installing.";
			install(version);
			m_modVersions.removeAll(version);
		}

		if (!version->url.isValid())
		{
			m_modVersions.removeAll(version);
		}
	}

	if (checkForIsDone())
	{
		QMessageBox::information(this, tr("Information"), tr("Nothing to do"));
		return QDialog::Accepted;
	}

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

	ui->webModsProgressBar->setMaximum(m_modVersions.size());
	ui->webModsProgressBar->setValue(0);
	downloadNextMod();

	return QDialog::exec();
}

void QuickModInstallDialog::setInitialMods(const QList<QuickModPtr> mods)
{
	m_initialMods = mods;
}

void QuickModInstallDialog::downloadNextMod()
{
	if (checkForIsDone())
	{
		return;
	}

	if (m_modVersions.isEmpty())
	{
		return;
	}

	auto version = m_modVersions.takeFirst();
	ui->webModsProgressBar->setValue(ui->webModsProgressBar->value() + 1);

	QLOG_INFO() << "Downloading " << version->name() << "(" << version->url.toString() << ")";

	auto navigator = new WebDownloadNavigator(this);
	navigator->setProperty("version", QVariant::fromValue(version));
	connect(navigator, &WebDownloadNavigator::caughtUrl, this,
			&QuickModInstallDialog::urlCaught);
	navigator->load(version->url);
	ui->web->addWidget(navigator);
}

bool QuickModInstallDialog::checkForIsDone()
{
	if (m_downloadingUrls.isEmpty() && ui->web->count() == 0 && m_modVersions.isEmpty())
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

	ui->web->removeWidget(navigator);
	navigator->deleteLater();
	if (ui->web->count() == 0)
	{
		ui->tabWidget->setCurrentWidget(ui->downloads);
	}
}

void QuickModInstallDialog::processReply(QNetworkReply *reply, QuickModVersionPtr version)
{
	m_downloadingUrls.append(reply->url());

	auto item = new QTreeWidgetItem(ui->progressList);
	item->setText(0, version->mod->name());
	item->setIcon(0, version->mod->icon());
	item->setText(1, version->name());
	item->setText(2, reply->url().toString(QUrl::PrettyDecoded));
	item->setData(3, ExtraRoles::IgnoreRole, false);
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
		item->setText(3, tr("Success: Installed successfully"));
		item->setData(3, Qt::ForegroundRole, QColor(Qt::darkGreen));
	}

	checkForIsDone();
}

#include "QuickModInstallDialog.moc"
