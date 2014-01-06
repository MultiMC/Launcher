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
#include "gui/dialogs/ChooseQuickModVersionDialog.h"
#include "gui/dialogs/ProgressDialog.h"
#include "logic/lists/QuickModsList.h"
#include "logic/BaseInstance.h"
#include "logic/net/ByteArrayDownload.h"
#include "logic/net/NetJob.h"
#include "MultiMC.h"
#include "depends/settings/settingsobject.h"
#include "depends/quazip/JlCompress.h"

Q_DECLARE_METATYPE(QTreeWidgetItem *)

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

	connect(MMC->quickmodslist().get(), &QuickModsList::modAdded, this,
			&QuickModInstallDialog::newModRegistered);

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
		if (ui->web->currentWidget())
		{
			qDebug() << "New index is" << ui->web->currentIndex() << "with the current URL"
					 << qobject_cast<WebDownloadNavigator *>(ui->web->currentWidget())->url();
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
		if (ui->web->currentWidget())
		{
			qDebug() << "New index is" << ui->web->currentIndex() << "with the current URL"
					 << qobject_cast<WebDownloadNavigator *>(ui->web->currentWidget())->url();
		}
	});
}

QuickModInstallDialog::~QuickModInstallDialog()
{
	delete ui;
}

int QuickModInstallDialog::exec()
{
	showMaximized();
	return QDialog::exec();
}

void QuickModInstallDialog::checkForIsDone()
{
	if (m_trackedMods.isEmpty() && m_pendingDependencyUrls.isEmpty())
	{
		ui->finishButton->setEnabled(true);
	}
	else
	{
		ui->finishButton->setDisabled(true);
	}
}

bool QuickModInstallDialog::addMod(QuickMod *mod, bool isInitial, const QString &versionFilter)
{
	if (m_pendingInstallations.contains(mod->name()))
	{
		return false;
	}
	m_pendingInstallations.append(mod->name());

	auto secondStageAddMod = [this](QuickMod *mod, bool isInitial, const QString &versionFilter)
	{
		ChooseQuickModVersionDialog dialog(this);
		dialog.setCanCancel(isInitial);
		dialog.setMod(mod, m_instance, versionFilter);
		if (dialog.exec() == QDialog::Rejected)
		{
			if (isInitial)
			{
				reject();
			}
			return false;
		}

		// TODO any installed version should prevent another version from being installed
		if (MMC->quickmodslist()->isModMarkedAsInstalled(mod, dialog.version(), m_instance))
		{
			if (isInitial)
			{
				accept();
			}
			return false;
		}

		if (MMC->quickmodslist()->isModMarkedAsExists(mod, dialog.version()))
		{
			install(mod, dialog.version());
			if (isInitial)
			{
				accept();
			}
			return false;
		}

		foreach(const QString & dep, mod->version(dialog.version()).dependencies.keys())
		{
			if (!m_pendingDependencyUrls.contains(mod->references()[dep])
					&& mod->references().contains(dep)
					&& !mod->references()[dep].isEmpty())
			{
				ui->dependencyLogEdit->appendHtml(
					QString("Fetching dependency URL %1...<br/>")
						.arg(mod->references()[dep].toString(QUrl::PrettyDecoded)));
				MMC->quickmodslist()->registerMod(mod->references()[dep]);
				m_pendingDependencyUrls.append(mod->references()[dep]);
			}
		}

		auto navigator = new WebDownloadNavigator(this);
		connect(navigator, &WebDownloadNavigator::caughtUrl, this,
				&QuickModInstallDialog::urlCaught);
		navigator->load(mod->version(dialog.version()).url);
		m_webModMapping.insert(navigator, qMakePair(mod, dialog.version()));
		ui->web->addWidget(navigator);

		if (isInitial)
		{
			m_initialMod = mod;
		}
		m_trackedMods.insert(mod, dialog.version());

		show();
		activateWindow();
		return true;
	};

	if (mod->verifyUrl().isValid())
	{
		NetJobPtr job = NetJobPtr(new NetJob("Verify QuickMod file"));
		ByteArrayDownloadPtr ptr = ByteArrayDownload::make(mod->verifyUrl());
		job->addNetAction(ptr);
		ProgressDialog dialog(this);
		if (dialog.exec(job.get()) != QDialog::Accepted)
		{
			return false;
		}
		const QByteArray actual = ptr->m_data;
		const QByteArray expected = mod->hash().toHex();
		if (actual == expected)
		{
			if (!MMC->quickmodslist()->isWebsiteTrusted(mod->verifyUrl()))
			{
				QMessageBox box(this);
				box.setWindowTitle(tr("Trust website?"));
				box.setText(tr("The QuickMod %1 verifies against %2.\nTrust %2?")
								.arg(mod->name(), mod->verifyUrl().host()));
				box.setIcon(QMessageBox::Question);
				QPushButton *once = box.addButton(tr("Once"), QMessageBox::AcceptRole);
				QPushButton *always = box.addButton(tr("Always"), QMessageBox::AcceptRole);
				QPushButton *no = box.addButton(QMessageBox::No);
				box.setDefaultButton(always);
				box.exec();
				if (box.clickedButton() == no)
				{
					return false;
				}
				if (box.clickedButton() == always)
				{
					MMC->quickmodslist()->setWebsiteTrusted(mod->verifyUrl(), true);
				}
			}
			return secondStageAddMod(mod, isInitial, versionFilter);
		}
		else
		{
			QMessageBox box(this);
			box.setWindowTitle(tr("Trust website?"));
			box.setText(tr("The QuickMod %1 does NOT verify against %2.")
							.arg(mod->name(), mod->verifyUrl().host()));
			box.setInformativeText(tr("You may add a one-time exception, but it's strictly "
									  "recommended that you don't."));
			box.setIcon(QMessageBox::Warning);
			QPushButton *exception =
				box.addButton(tr("Add one-time exception"), QMessageBox::AcceptRole);
			QPushButton *no = box.addButton(QMessageBox::No);
			box.setDefaultButton(no);
			box.exec();
			if (box.clickedButton() == exception)
			{
				return secondStageAddMod(mod, isInitial, versionFilter);
			}
			return false;
		}
	}
	else
	{
		return secondStageAddMod(mod, isInitial, versionFilter);
	}
}

void QuickModInstallDialog::urlCaught(QNetworkReply *reply)
{
	auto navigator = qobject_cast<WebDownloadNavigator *>(sender());
	auto mod = m_webModMapping[navigator].first;
	auto version = mod->version(m_webModMapping[navigator].second);

	auto item = new QTreeWidgetItem(ui->progressList);
	item->setText(0, mod->name());
	item->setIcon(0, mod->icon());
	item->setText(1, version.name);
	item->setText(2, reply->url().toString(QUrl::PrettyDecoded));
	item->setData(3, ExtraRoles::IgnoreRole, false);
	reply->setProperty("item", QVariant::fromValue(item));
	reply->setProperty("mod", QVariant::fromValue(mod));
	reply->setProperty("versionIndex", m_webModMapping[navigator].second);

	connect(reply, &QNetworkReply::downloadProgress, this,
			&QuickModInstallDialog::downloadProgress);
	connect(reply, &QNetworkReply::finished, this, &QuickModInstallDialog::downloadCompleted);

	m_webModMapping.remove(navigator);
	ui->web->removeWidget(navigator);
	navigator->deleteLater();
	if (ui->web->count() == 0)
	{
		ui->tabWidget->setCurrentWidget(ui->downloads);
	}
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
	auto mod = reply->property("mod").value<QuickMod *>();
	auto versionIndex = reply->property("versionIndex").toInt();
	QDir dir;
	bool extract = false;
	switch (mod->type())
	{
	case QuickMod::ForgeMod:
		dir = dirEnsureExists(MMC->settings()->get("CentralModsDir").toString(), "mods");
		extract = false;
		break;
	case QuickMod::ForgeCoreMod:
		dir = dirEnsureExists(MMC->settings()->get("CentralModsDir").toString(), "coremods");
		extract = false;
		break;
	case QuickMod::ResourcePack:
		Q_ASSERT_X(false, __func__, "Not implemented");
		break;
	case QuickMod::ConfigPack:
		Q_ASSERT_X(false, __func__, "Not implemented");
		break;
	case QuickMod::Group:
		item->setText(3, tr("Sucess: Installed successfully"));
		item->setData(3, Qt::ForegroundRole, QColor(Qt::green));

		m_trackedMods.remove(mod);
		checkForIsDone();
		return;
	}

	QByteArray data = reply->readAll();

	if (!mod->version(versionIndex).checksum.isNull() &&
		QCryptographicHash::hash(data, mod->version(versionIndex).checksum_algorithm).toHex() !=
			mod->version(versionIndex).checksum)
	{
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
		reply->seek(0);
		file.write(data);
		file.close();
		reply->close();
		reply->deleteLater();

		MMC->quickmodslist()->markModAsExists(mod, versionIndex, file.fileName());

		install(mod, versionIndex);
	}

	item->setText(3, tr("Sucess: Installed successfully"));
	item->setData(3, Qt::ForegroundRole, QColor(Qt::green));

	m_trackedMods.remove(mod);
	checkForIsDone();
}

void QuickModInstallDialog::newModRegistered(QuickMod *newMod)
{
	bool haveAdded = false;
	auto checkModForDependencies = [ this, haveAdded, newMod ](QuickMod * mod, int versionIndex)
																  ->bool
	{
		QuickMod::Version version = mod->version(versionIndex);
		if (version.dependencies.contains(newMod->name()))
		{
			ui->dependencyLogEdit->appendHtml(QString("Resolved dependency from %1 to %2<br/>")
												  .arg(mod->name(), newMod->name()));
			if (!haveAdded)
			{
				addMod(newMod, false, version.dependencies[newMod->name()]);
				return true;
			}
		}
		return false;
	};
	checkModForDependencies(m_initialMod, m_trackedMods[m_initialMod]);
	foreach(QuickMod * mod, m_trackedMods.keys())
	{
		if (checkModForDependencies(mod, m_trackedMods[mod]))
		{
			haveAdded = true;
		}
	}
	m_pendingDependencyUrls.removeAll(newMod->updateUrl());
	checkForIsDone();
}

void QuickModInstallDialog::install(QuickMod *mod, const int versionIndex)
{
	QDir finalDir;
	switch (mod->type())
	{
	case QuickMod::ForgeMod:
		finalDir = dirEnsureExists(m_instance->minecraftRoot(), "mods");
		break;
	case QuickMod::ForgeCoreMod:
		finalDir = dirEnsureExists(m_instance->minecraftRoot(), "coremods");
		break;
	case QuickMod::ResourcePack:
		Q_ASSERT_X(false, __func__, "Not implemented");
		break;
	case QuickMod::ConfigPack:
		Q_ASSERT_X(false, __func__, "Not implemented");
		break;
	case QuickMod::Group:
		return;
	}
	const QString file = MMC->quickmodslist()->existingModFile(mod, versionIndex);
	const QString dest = finalDir.absoluteFilePath(QFileInfo(file).fileName());
	if (!QFile::copy(file, dest))
	{
		QMessageBox::critical(this, tr("Error"),
							  tr("Error deploying %1 to %2").arg(file, dest));
		return;
	}
	MMC->quickmodslist()->markModAsInstalled(mod, versionIndex, dest, m_instance);
}

#include "QuickModInstallDialog.moc"
