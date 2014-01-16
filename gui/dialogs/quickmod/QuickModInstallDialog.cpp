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
#include "gui/dialogs/quickmod/ChooseQuickModVersionDialog.h"
#include "gui/dialogs/ProgressDialog.h"
#include "gui/dialogs/VersionSelectDialog.h"
#include "logic/quickmod/QuickModsList.h"
#include "logic/quickmod/QuickMod.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModDependencyDownloadTask.h"
#include "logic/quickmod/QuickModDependencyResolver.h"
#include "logic/BaseInstance.h"
#include "logic/net/ByteArrayDownload.h"
#include "logic/net/NetJob.h"
#include "MultiMC.h"
#include "depends/settings/settingsobject.h"
#include "depends/quazip/JlCompress.h"

Q_DECLARE_METATYPE(QTreeWidgetItem *)

// TODO install forge / liteloader
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

QuickModInstallDialog::QuickModInstallDialog(BaseInstance *instance, QWidget *parent)
	: QDialog(parent), ui(new Ui::QuickModInstallDialog), m_instance(instance)
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
		connect(&resolver, &QuickModDependencyResolver::totallyUnknownError, [this, &error]()
		{
			error = true;
			QListWidgetItem *item = new QListWidgetItem(tr("Unknown error"));
			item->setTextColor(Qt::red);
			ui->dependencyListWidget->addItem(item);
		});
		connect(&resolver, &QuickModDependencyResolver::didNotSelectVersionError,
				[this, &error](QuickModVersionPtr from, const QString &to)
		{
			error = true;
			QListWidgetItem *item = new QListWidgetItem(tr("Didn't select a version while resolving from %1 (%2) to %3")
														.arg(from->mod->name(), from->name(), to));
			item->setTextColor(Qt::red);
			ui->dependencyListWidget->addItem(item);
		});
		connect(&resolver, &QuickModDependencyResolver::unresolvedDependency,
				[this, &error](QuickModVersionPtr from, const QString &to)
		{
			error = true;
			QListWidgetItem *item = new QListWidgetItem(tr("The dependency from %1 (%2) to %3 cannot be resolved")
														.arg(from->mod->name(), from->name(), to));
			item->setTextColor(Qt::red);
			ui->dependencyListWidget->addItem(item);
		});
		connect(&resolver, &QuickModDependencyResolver::resolvedDependency,
				[this, &error](QuickModVersionPtr from, QuickModVersionPtr to)
		{
			QListWidgetItem *item = new QListWidgetItem(tr("Successfully resolved dependency from %1 (%2) to %3 (%4)")
														.arg(from->mod->name(), from->name(), to->mod->name(), to->name()));
			item->setTextColor(Qt::darkGreen);
			ui->dependencyListWidget->addItem(item);
		});
		m_modVersions = resolver.resolve(m_initialMods);
		if (!error)
		{
			ui->tabWidget->setCurrentIndex(1);
		}
	}

	foreach(QuickModVersionPtr version, m_modVersions)
	{
		// TODO any installed version should prevent another version from being installed
		if (MMC->quickmodslist()->isModMarkedAsInstalled(version->mod, version, m_instance))
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

	downloadNextMod();

	return QDialog::exec();
}

void QuickModInstallDialog::setInitialMods(const QList<QuickMod *> mods)
{
	m_initialMods = mods;
}

void QuickModInstallDialog::downloadNextMod()
{
	if (checkForIsDone())
	{
		return;
	}

	m_currentVersion = m_modVersions.takeFirst();

	QLOG_INFO() << "Downloading " << m_currentVersion->name() << "("
				<< m_currentVersion->url.toString() << ")";

	auto navigator = new WebDownloadNavigator(this);
	connect(navigator, &WebDownloadNavigator::caughtUrl, this,
			&QuickModInstallDialog::urlCaught);
	navigator->load(m_currentVersion->url);
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

void QuickModInstallDialog::urlCaught(QNetworkReply *reply)
{
	QLOG_INFO() << "Caught " << reply->url().toString();
	downloadNextMod();

	auto navigator = qobject_cast<WebDownloadNavigator *>(sender());

	processReply(reply, m_currentVersion);

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

static QString fileName(const QUrl &url)
{
	const QString path = url.path();
	return path.mid(path.lastIndexOf('/') + 1);
}
static bool saveDeviceToFile(const QByteArray &data, const QString &file)
{
	QFile out(file);
	if (!out.open(QFile::WriteOnly | QFile::Truncate))
	{
		return false;
	}
	return data.size() == out.write(data);
}
static QDir dirEnsureExists(const QString &dir, const QString &path)
{
	QDir dir_ = QDir::current();

	// first stage
	if (!dir_.exists(dir))
	{
		dir_.mkpath(dir);
	}
	dir_.cd(dir);

	// second stage
	if (!dir_.exists(path))
	{
		dir_.mkpath(path);
	}
	dir_.cd(path);

	return dir_;
}

void QuickModInstallDialog::downloadCompleted()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
	auto item = reply->property("item").value<QTreeWidgetItem *>();
	item->setData(3, ExtraRoles::IgnoreRole, true);
	item->setText(3, tr("Installing..."));
	auto version = reply->property("version").value<QuickModVersionPtr>();
	QDir dir;
	bool extract = false;
	switch (version->installType)
	{
	case QuickModVersion::ForgeMod:
		dir = dirEnsureExists(MMC->settings()->get("CentralModsDir").toString(), "mods");
		extract = false;
		break;
	case QuickModVersion::ForgeCoreMod:
		dir = dirEnsureExists(MMC->settings()->get("CentralModsDir").toString(), "coremods");
		extract = false;
		break;
	case QuickModVersion::Extract:
		Q_ASSERT_X(false, __func__, "Not implemented");
		return;
	case QuickModVersion::ConfigPack:
		Q_ASSERT_X(false, __func__, "Not implemented");
		return;
	case QuickModVersion::Group:
		item->setText(3, tr("Success: Installed successfully"));
		item->setData(3, Qt::ForegroundRole, QColor(Qt::green));

		checkForIsDone();
		return;
	}

	checkForIsDone();

	QByteArray data = reply->readAll();

	const QByteArray actual =
		QCryptographicHash::hash(data, version->checksum_algorithm).toHex();
	if (!version->checksum.isNull() && actual != version->checksum)
	{
		QLOG_INFO() << "Checksum missmatch for " << version->mod->uid()
					<< ". Actual: " << actual << " Expected: " << version->checksum;
		item->setText(3, tr("Error: Checksum mismatch"));
		item->setData(3, Qt::ForegroundRole, QColor(Qt::red));
		return;
	}

	if (extract)
	{
		const QString path = reply->url().path();
		// TODO more file formats. KArchive?
		if (path.endsWith(".zip"))
		{
			const QString zipFileName =
				QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
					.absoluteFilePath(fileName(reply->url()));
			if (saveDeviceToFile(data, zipFileName))
			{
				JlCompress::extractDir(zipFileName, dir.absolutePath());
			}
			else
			{
				item->setText(3, tr("Error: Checksum mismatch"));
				item->setData(3, Qt::ForegroundRole, QColor(Qt::red));
				return;
			}
		}
		else
		{
			item->setText(3, tr("Error: Trying to extract an unknown file type %1")
								 .arg(reply->url().toString(QUrl::PrettyDecoded)));
			item->setData(3, Qt::ForegroundRole, QColor(Qt::red));
			return;
		}
	}
	else
	{
		QFile file(dir.absoluteFilePath(fileName(reply->url())));
		if (!file.open(QFile::WriteOnly | QFile::Truncate))
		{
			item->setText(
				3, tr("Error: Trying to save %1: %2").arg(file.fileName(), file.errorString()));
			item->setData(3, Qt::ForegroundRole, QColor(Qt::red));
			return;
		}
		file.write(data);
		file.close();
		reply->close();
		reply->deleteLater();

		MMC->quickmodslist()->markModAsExists(version->mod, version, file.fileName());

		install(version);
	}

	item->setText(3, tr("Success: Installed successfully"));
	item->setData(3, Qt::ForegroundRole, QColor(Qt::darkGreen));
}

void QuickModInstallDialog::install(const QuickModVersionPtr version)
{
	QDir finalDir;
	switch (version->installType)
	{
	case QuickModVersion::ForgeMod:
		finalDir = dirEnsureExists(m_instance->minecraftRoot(), "mods");
		break;
	case QuickModVersion::ForgeCoreMod:
		finalDir = dirEnsureExists(m_instance->minecraftRoot(), "coremods");
		break;
	case QuickModVersion::Extract:
		Q_ASSERT_X(false, __func__, "Not implemented");
		return;
	case QuickModVersion::ConfigPack:
		Q_ASSERT_X(false, __func__, "Not implemented");
		return;
	case QuickModVersion::Group:
		return;
	}
	const QString file = MMC->quickmodslist()->existingModFile(version->mod, version);
	const QString dest = finalDir.absoluteFilePath(QFileInfo(file).fileName());
	if (!QFile::copy(file, dest))
	{
		QMessageBox::critical(this, tr("Error"),
							  tr("Error deploying %1 to %2").arg(file, dest));
		return;
	}
	MMC->quickmodslist()->markModAsInstalled(version->mod, version, dest, m_instance);
}

#include "QuickModInstallDialog.moc"
