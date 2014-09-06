/* Copyright 2014 MultiMC Contributors
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

#include "QuickModCreateFromInstanceDialog.h"
#include "ui_QuickModCreateFromInstanceDialog.h"

#include <QFileDialog>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QUrl>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCompleter>

#include "logic/settings/SettingsObject.h"
#include "logic/OneSixInstance.h"
#include "logic/auth/MojangAccountList.h"
#include "logic/MMCJson.h"
#include "logic/net/ByteArrayUpload.h"
#include "logic/net/ByteArrayDownload.h"
#include "logic/net/NetJob.h"
#include "logic/quickmod/QuickMod.h"
#include "logic/quickmod/QuickModBuilder.h"
#include "MultiMC.h"

static int addRowToTableWidget(QTableWidget *widget)
{
	const int row = widget->rowCount();
	widget->insertRow(row);
	widget->setItem(row, 0, new QTableWidgetItem);
	widget->setItem(row, 1, new QTableWidgetItem);
	widget->setCurrentCell(row, 0);
	return row;
}

class QuickModUrlValidator : public QValidator
{
	Q_OBJECT
public:
	QuickModUrlValidator(QObject *parent = 0) : QValidator(parent)
	{
	}

	State validate(QString &string, int &pos) const override
	{
		if (string == "%{ImgurUpload:Icon}")
		{
			return Acceptable;
		}
		else if (string.startsWith("%{"))
		{
			return Intermediate;
		}

		const QUrl url = QUrl::fromUserInput(string);
		if (url.isValid() && !url.isRelative())
		{
			return Acceptable;
		}
		return Intermediate;
	}
};

class UrlsDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	explicit UrlsDelegate(QObject *parent) : QStyledItemDelegate(parent)
	{
	}

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
						  const QModelIndex &index) const
	{
		if (index.column() != 0)
		{
			QWidget *widget = QStyledItemDelegate::createEditor(parent, option, index);
			if (QLineEdit *edit = qobject_cast<QLineEdit *>(widget))
			{
				edit->setValidator(new QuickModUrlValidator(edit));
			}
			return widget;
		}
		QComboBox *box = new QComboBox(parent);
		box->setFrame(false);
		for (const auto type : QuickMod::urlTypes())
		{
			box->addItem(QuickMod::humanUrlId(type), QuickMod::urlId(type));
		}
		box->showPopup();
		return box;
	}

	void setEditorData(QWidget *editor, const QModelIndex &index) const
	{
		if (index.column() != 0)
		{
			QStyledItemDelegate::setEditorData(editor, index);
		}
		else
		{
			QComboBox *box = qobject_cast<QComboBox *>(editor);
			box->setCurrentIndex(box->findData(index.model()->data(index, Qt::UserRole)));
		}
	}
	void setModelData(QWidget *editor, QAbstractItemModel *model,
					  const QModelIndex &index) const
	{
		if (index.column() != 0)
		{
			QStyledItemDelegate::setEditorData(editor, index);
		}
		else
		{
			QComboBox *box = qobject_cast<QComboBox *>(editor);
			model->setData(index, box->itemData(box->currentIndex()), Qt::UserRole);
			model->setData(index, box->currentText(), Qt::DisplayRole);
		}
	}

	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
							  const QModelIndex &index) const
	{
		if (index.column() != 0)
		{
			QStyledItemDelegate::updateEditorGeometry(editor, option, index);
		}
		else
		{
			editor->setGeometry(option.rect);
		}
	}
};

QuickModCreateFromInstanceDialog::QuickModCreateFromInstanceDialog(
	std::shared_ptr<OneSixInstance> instance, QWidget *parent)
	: QDialog(parent), ui(new Ui::QuickModCreateFromInstanceDialog), m_instance(instance)
{
	ui->setupUi(this);

	ui->progressLabel->hide();
	ui->progressBar->hide();

	m_tagsModel = new QStringListModel(this);
	m_categoriesModel = new QStringListModel(this);

	ui->tagsList->setModel(m_tagsModel);
	ui->categoriesList->setModel(m_categoriesModel);
	ui->urlsTable->setItemDelegate(new UrlsDelegate(this));

	ui->usernameEdit->setText(m_instance->settings().get("UploadUsername").toString());
	ui->passwordEdit->setText(m_instance->settings().get("UploadPassword").toString());

	ui->licenseEdit->setCompleter(new QCompleter(
	{
		"Apache 1.0", "Apache 1.1", "Apache 2.0", "Artistic 1.0", "Artistic 2.0",
			"BSD 2 Clause", "BSD 3 Clause", "BSD 4 Clause", "CC BY 1.0", "CC BY 2.0",
			"CC BY 2.5", "CC BY 3.0", "CC BY-ND 1.0", "CC BY ND 2.0", "CC BY ND 2.5",
			"CC BY ND 3.0", "CC BY NC 1.0", "CC BY NC 2.0", "CC BY NC 2.5", "CC BY NC 3.0",
			"CC BY NC ND 1.0", "CC BY NC ND 2.0", "CC BY NC ND 2.5", "CC BY NC ND 3.0",
			"CC BY NC SA 1.0", "CC BY NC SA 2.0", "CC BY NC SA 2.5", "CC BY NC SA 3.0",
			"CC BY SA 1.0", "CC BY SA 2.0", "CC BY SA 2.5", "CC BY SA 3.0", "CC0 1.0",
			"AGPL 1.0", "AGPL 3.0", "GPL 1.0", "GPL 2.0", "GPL 3.0", "LGPL 2.1", "LGPL 3.0",
			"LGPL 2.0", "MMPL-1.0"
	}, ui->licenseEdit));

	connect(ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
			this, &QuickModCreateFromInstanceDialog::resetValues);

	resetValues();
}

QuickModCreateFromInstanceDialog::~QuickModCreateFromInstanceDialog()
{
	m_instance->settings().set("UploadUsername", ui->usernameEdit->text());
	m_instance->settings().set("UploadPassword", ui->passwordEdit->text());
	delete ui;
}

void QuickModCreateFromInstanceDialog::resetValues()
{
	ui->localBtn->click();

	ui->uidEdit->clear();
	ui->repoEdit->clear();
	ui->versionEdit->clear();
	ui->versionTypeEdit->clear();
	ui->licenseEdit->clear();
	ui->nameEdit->setText(m_instance->name());
	ui->descriptionEdit->setText(m_instance->notes());

	m_tagsModel->setStringList(QStringList());
	m_categoriesModel->setStringList(QStringList() << "Mod Packs" << m_instance->group());

	resetTables();

	const auto account = MMC->accounts()->activeAccount();
	const QString defaultUserName = account ? account->currentProfile()->name : QString();
	if (!defaultUserName.isNull())
	{
		const int row = addRowToTableWidget(ui->authorsTable);
		ui->authorsTable->item(row, 0)->setText(tr("Author"));
		ui->authorsTable->item(row, 1)->setText(defaultUserName);
	}
}

void QuickModCreateFromInstanceDialog::resetTables()
{
	ui->authorsTable->clear();
	ui->urlsTable->clear();

	ui->authorsTable->setHorizontalHeaderLabels(QStringList() << tr("Type") << tr("Name"));
	ui->urlsTable->setHorizontalHeaderLabels(QStringList() << tr("Type") << tr("URL"));
}

bool QuickModCreateFromInstanceDialog::hasVersion(const QString &name) const
{
	for (const auto version : m_otherVersions)
	{
		if (version->version().toString() == name)
		{
			return true;
		}
	}
	return false;
}

void QuickModCreateFromInstanceDialog::createVersion(QuickModVersionBuilder builder) const
{
	builder.setCompatibleMCVersions(QStringList() << m_instance->currentVersionId());
	for (const auto lib_ : m_instance->getFullVersion()->libraries)
	{
		OneSixLibraryPtr lib = lib_;
		if (lib->artifactId() == "forge" || lib->artifactId() == "minecraftforge")
		{
			builder.setForgeVersionFilter(lib->version());
		}
	}
	builder.setInstallType(QuickModVersion::Group);
	builder.setName(ui->versionEdit->text());
	if (!ui->versionTypeEdit->text().isEmpty())
	{
		builder.setType(ui->versionTypeEdit->text());
	}
	const auto quickmods = m_instance->getFullVersion()->quickmods;
	for (auto it = quickmods.constBegin(); it != quickmods.constEnd(); ++it)
	{
		if (it.value().second)
		{
			builder.addDependency(it.key(), it.value().first);
		}
	}
	builder.build();
}

void QuickModCreateFromInstanceDialog::start(ProgressProvider *provider)
{
	Q_ASSERT(!provider->isRunning());

	auto finish = [this]()
	{
		ui->metadataBox->setEnabled(true);
		ui->buttonBox->setEnabled(true);
		ui->settingsWidget->setEnabled(true);
		ui->progressLabel->hide();
		ui->progressBar->hide();
	};

	connect(provider, &ProgressProvider::started, [this]()
	{
		ui->metadataBox->setEnabled(false);
		ui->buttonBox->setEnabled(false);
		ui->settingsWidget->setEnabled(false);
		ui->progressLabel->show();
		ui->progressBar->show();
	});
	connect(provider, &ProgressProvider::progress, [this](qint64 current, qint64 total)
	{
		ui->progressBar->setMaximum(total);
		ui->progressBar->setValue(current);
	});
	connect(provider, &ProgressProvider::status, ui->progressLabel, &QLabel::setText);
	connect(provider, &ProgressProvider::succeeded, [this, finish]()
	{ finish(); });
	connect(provider, &ProgressProvider::failed, [this, finish](const QString & reason)
	{
		QMessageBox::critical(this, tr("Error"), reason);
		finish();
	});
	provider->start();
}

void QuickModCreateFromInstanceDialog::on_browseBtn_clicked()
{
	static const QString title = tr("Select QuickMod File");
	static const QString filters =
		tr("QuickMod files (*.quickmod *.qm);;JSON files (*.js *.json);;All Files (*)");
	if (ui->remoteBtn->isChecked())
	{
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
		const QUrl url = QFileDialog::getOpenFileUrl(
			this, title, m_instance->settings().get("LastQuickModUrl").toUrl(), filters);
#else
		const QUrl url = QUrl(QFileDialog::getOpenFileName(
			this, title, m_instance->settings().get("LastQuickModUrl").toUrl().toLocalFile(),
			filters));
#endif
		if (url.isValid())
		{
			m_instance->settings().set("LastQuickModUrl", url);
			ui->endpointEdit->setText(url.toString());
		}
	}
	else
	{
		const QString filename = QFileDialog::getOpenFileName(
			this, title, m_instance->settings().get("LastQuickModFile").toString(), filters);
		if (!filename.isNull())
		{
			m_instance->settings().set("LastQuickModFile", filename);
			ui->endpointEdit->setText(filename);
		}
	}
}

void QuickModCreateFromInstanceDialog::on_remoteBtn_clicked()
{
	ui->loginBox->setEnabled(true);
	ui->importBtn->setText(tr("Download"));
	ui->exportBtn->setText(tr("Upload"));

	ui->endpointEdit->setText(m_instance->settings().get("LastQuickModUrl").toUrl().toString());
}

void QuickModCreateFromInstanceDialog::on_localBtn_clicked()
{
	ui->loginBox->setEnabled(false);
	ui->importBtn->setText(tr("Open"));
	ui->exportBtn->setText(tr("Save"));

	ui->endpointEdit->setText(m_instance->settings().get("LastQuickModFile").toString());
}

void QuickModCreateFromInstanceDialog::on_exportBtn_clicked()
{
	try
	{
		if (ui->uidEdit->text().isEmpty())
		{
			throw MMCError("You need to specify a UID");
		}
		if (ui->repoEdit->text().isEmpty())
		{
			throw MMCError("You need to specify a repository");
		}
		if (ui->nameEdit->text().isEmpty())
		{
			throw MMCError("You need to specify a name");
		}
		if (ui->updateUrlEdit->text().isEmpty())
		{
			throw MMCError("You need to specify an update URL");
		}
		if (ui->versionEdit->text().isEmpty())
		{
			throw MMCError("You need to specify a version name");
		}
		if (hasVersion(ui->versionEdit->text()))
		{
			int answer = QMessageBox::question(
				this, tr("Overwrite?"),
				tr("There already exist a version with the name %1 in %2, overwrite?"),
				QMessageBox::Yes, QMessageBox::No);
			if (answer == QMessageBox::No)
			{
				return;
			}
		}

		const QString fileName = filename();
		if (!fileName.isNull())
		{
			QFile file(fileName);
			if (!file.open(QFile::WriteOnly))
			{
				throw MMCError(tr("Unable to open %1 for writing: %2")
								   .arg(file.fileName(), file.errorString()));
			}
			file.write(toJson());
		}
		else
		{
			const QUrl url = QUrl::fromUserInput(ui->endpointEdit->text());
			if (!url.isValid())
			{
				throw MMCError("Invalid URL");
			}
			NetJob *job = new NetJob(tr("Upload QuickMod"));
			auto download = ByteArrayUpload::make(url, toJson());
			connect(MMC->qnam().get(), &QNetworkAccessManager::authenticationRequired,
					[this, download](QNetworkReply * reply, QAuthenticator * authenticator)
			{
				if (download->m_reply.get() == reply)
				{
					authenticator->setUser(ui->usernameEdit->text());
					authenticator->setPassword(ui->passwordEdit->text());
				}
			});
			job->addNetAction(download);
			start(job);
		}
	}
	catch (MMCError &error)
	{
		QMessageBox::critical(this, tr("Error"), error.cause());
	}
}

void QuickModCreateFromInstanceDialog::on_importBtn_clicked()
{
	try
	{
		const QString fileName = filename();
		if (!fileName.isNull())
		{
			QFile file(fileName);
			if (!file.open(QFile::ReadOnly))
			{
				throw MMCError(tr("Unable to open %1 for reading: %2")
								   .arg(file.fileName(), file.errorString()));
			}
			fromJson(file.readAll());
		}
		else
		{
			const QUrl url = QUrl::fromUserInput(ui->endpointEdit->text());
			if (!url.isValid())
			{
				throw MMCError("Invalid URL");
			}
			NetJob *job = new NetJob(tr("Download QuickMod"));
			auto download = ByteArrayDownload::make(url);
			download->m_followRedirects = true;
			connect(MMC->qnam().get(), &QNetworkAccessManager::authenticationRequired,
					[this, download](QNetworkReply * reply, QAuthenticator * authenticator)
			{
				if (download->m_reply.get() == reply)
				{
					authenticator->setUser(ui->usernameEdit->text());
					authenticator->setPassword(ui->passwordEdit->text());
				}
			});
			connect(download.get(), &ByteArrayDownload::succeeded, [this, download](int)
			{ fromJson(download->m_data); });
			job->addNetAction(download);
			start(job);
		}
	}
	catch (MMCError &error)
	{
		QMessageBox::critical(this, tr("Error"), error.cause());
	}
}

void QuickModCreateFromInstanceDialog::on_authorsAddBtn_clicked()
{
	addRowToTableWidget(ui->authorsTable);
}

void QuickModCreateFromInstanceDialog::on_authorsRemoveBtn_clicked()
{
	auto table = ui->authorsTable;
	if (table->currentRow() != -1)
	{
		table->removeRow(table->currentRow());
	}
}

void QuickModCreateFromInstanceDialog::on_tagsAddBtn_clicked()
{
	const int row = m_tagsModel->rowCount();
	m_tagsModel->insertRow(row);
	ui->tagsList->setCurrentIndex(m_tagsModel->index(row));
	ui->tagsList->edit(ui->tagsList->currentIndex());
}

void QuickModCreateFromInstanceDialog::on_tagsRemoveBtn_clicked()
{
	const QModelIndex index = ui->tagsList->selectionModel()->currentIndex();
	if (!index.isValid())
	{
		return;
	}
	m_tagsModel->removeRow(index.row(), index.parent());
}

void QuickModCreateFromInstanceDialog::on_categoriesAddBtn_clicked()
{
	const int row = m_categoriesModel->rowCount();
	m_categoriesModel->insertRow(row);
	ui->categoriesList->setCurrentIndex(m_categoriesModel->index(row));
	ui->categoriesList->edit(ui->categoriesList->currentIndex());
}

void QuickModCreateFromInstanceDialog::on_categoriesRemoveBtn_clicked()
{
	const QModelIndex index = ui->categoriesList->selectionModel()->currentIndex();
	if (!index.isValid())
	{
		return;
	}
	m_categoriesModel->removeRow(index.row(), index.parent());
}

void QuickModCreateFromInstanceDialog::on_urlsAddBtn_clicked()
{
	auto table = ui->urlsTable;
	const int row = addRowToTableWidget(table);
	table->item(row, 0)->setData(Qt::UserRole, QuickMod::urlId(QuickMod::Website));
	table->item(row, 0)->setData(Qt::DisplayRole, QuickMod::humanUrlId(QuickMod::Website));
}

void QuickModCreateFromInstanceDialog::on_urlsRemoveBtn_clicked()
{
	auto table = ui->urlsTable;
	if (table->currentRow() != -1)
	{
		table->removeRow(table->currentRow());
	}
}

void QuickModCreateFromInstanceDialog::on_endpointEdit_textChanged(const QString &text)
{
	if (ui->localBtn->isChecked())
	{
		m_instance->settings().set("LastQuickModFile", text);
	}
	else
	{
		m_instance->settings().set("LastQuickModUrl", QUrl::fromUserInput(text));
	}
}

QString QuickModCreateFromInstanceDialog::filename() const
{
	const QUrl url = QUrl::fromUserInput(ui->endpointEdit->text());
	if (url.isValid())
	{
		return url.isLocalFile() ? url.toLocalFile() : QString();
	}
	return ui->endpointEdit->text();
}

QByteArray QuickModCreateFromInstanceDialog::toJson() const
{
	QuickModBuilder builder = QuickModBuilder();
	builder.setUid(ui->uidEdit->text());
	builder.setRepo(ui->repoEdit->text());
	builder.setName(ui->nameEdit->text());
	builder.setUpdateUrl(QUrl::fromUserInput(ui->updateUrlEdit->text()));
	builder.setDescription(ui->descriptionEdit->toPlainText());
	builder.setLicense(ui->licenseEdit->text());
	builder.setTags(m_tagsModel->stringList());
	builder.setCategories(m_categoriesModel->stringList());

	for (int i = 0; i < ui->authorsTable->rowCount(); ++i)
	{
		builder.addAuthor(ui->authorsTable->item(i, 0)->text(),
						  ui->authorsTable->item(i, 1)->text());
	}

	for (int i = 0; i < ui->urlsTable->rowCount(); ++i)
	{
		builder.addUrl(ui->urlsTable->item(i, 0)->data(Qt::UserRole).toString(),
					   ui->urlsTable->item(i, 1)->text());
	}

	createVersion(builder.addVersion());

	for (const auto version : m_otherVersions)
	{
		if (version->version().toString() == ui->nameEdit->text())
		{
			continue;
		}
		builder.addVersion(version);
	}

	return builder.build()->toJson();
}

void QuickModCreateFromInstanceDialog::fromJson(const QByteArray &json)
{
	resetValues();
	resetTables();
	QuickModPtr mod = std::make_shared<QuickMod>();
	mod->parse(mod, json);
	m_otherVersions = mod->versionsInternal();
	ui->uidEdit->setText(mod->uid().toString());
	ui->repoEdit->setText(mod->repo());
	ui->nameEdit->setText(mod->name());
	ui->descriptionEdit->setPlainText(mod->description());
	ui->licenseEdit->setText(mod->license());
	ui->updateUrlEdit->setText(mod->updateUrl().toString());
	m_tagsModel->setStringList(mod->tags());
	m_categoriesModel->setStringList(mod->categories());
	for (const auto type : QuickMod::urlTypes())
	{
		for (const auto url : mod->url(type))
		{
			const int row = addRowToTableWidget(ui->urlsTable);
			ui->urlsTable->item(row, 0)->setText(QuickMod::humanUrlId(type));
			ui->urlsTable->item(row, 0)->setData(Qt::UserRole, QuickMod::urlId(type));
			ui->urlsTable->item(row, 1)->setText(url.toString());
		}
	}
	for (const auto type : mod->authorTypes())
	{
		for (const auto author : mod->authors(type))
		{
			const int row = addRowToTableWidget(ui->authorsTable);
			ui->authorsTable->item(row, 0)->setText(type);
			ui->authorsTable->item(row, 1)->setText(author);
		}
	}
}

QStringList QuickModCreateFromInstanceDialog::reverseStringList(const QStringList &list)
{
	QStringList out = list;
	std::reverse(out.begin(), out.end());
	return out;
}

QStringList QuickModCreateFromInstanceDialog::variantListToStringList(const QVariantList &list)
{
	QStringList out;
	for (const auto item : list)
	{
		out.append(item.toString());
	}
	return out;
}

#include "QuickModCreateFromInstanceDialog.moc"
