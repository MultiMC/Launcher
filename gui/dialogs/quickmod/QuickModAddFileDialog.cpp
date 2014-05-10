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

#include "QuickModAddFileDialog.h"
#include "ui_QuickModAddFileDialog.h"

#include <QIcon>
#include <QFileDialog>
#include <QUrl>
#include <QCompleter>
#include <QFileSystemModel>

#include "MultiMC.h"
#include "gui/Platform.h"
#include "logic/quickmod/QuickModsList.h"
#include "modutils.h"

class FileValidator : public QValidator
{
	Q_OBJECT
public:
	FileValidator(QObject *parent = 0) : QValidator(parent)
	{
	}

	State validate(QString &input, int &pos) const
	{
		if (input.isEmpty())
		{
			return Intermediate;
		}
		for (auto file : input.split(';'))
		{
			if (!QFileInfo(file).isFile())
			{
				return Intermediate;
			}
		}
		return Acceptable;
	}
};
class UrlValidator : public QValidator
{
	Q_OBJECT
public:
	UrlValidator(QObject *parent = 0) : QValidator(parent)
	{
	}

	State validate(QString &input, int &pos) const
	{
		if (input.isEmpty())
		{
			return Intermediate;
		}
		return QUrl::fromUserInput(input).isValid() ? Acceptable : Intermediate;
	}
};

QuickModAddFileDialog::QuickModAddFileDialog(QWidget *parent)
	: QDialog(parent), ui(new Ui::QuickModAddFileDialog)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);

	ui->fileEdit->setValidator(new FileValidator(this));
	ui->urlEdit->setValidator(new UrlValidator(this));

	QCompleter *urlCompleter =
		new QCompleter(QStringList() << "github://02JanDal@QuickMod/index.json", this);
	urlCompleter->setCompletionMode(QCompleter::PopupCompletion);
	ui->urlEdit->setCompleter(urlCompleter);

	QFileSystemModel *fileModel = new QFileSystemModel(this);
	fileModel->setRootPath(QDir::currentPath());
	QCompleter *fileCompleter = new QCompleter(fileModel, this);
	fileCompleter->setCompletionMode(QCompleter::InlineCompletion);
	ui->fileEdit->setCompleter(fileCompleter);

	connect(ui->cancelButton, &QPushButton::clicked, this, &QuickModAddFileDialog::reject);

	ui->addButton->setEnabled(false);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
	ui->fileEdit->setClearButtonEnabled(true);
	ui->urlEdit->setClearButtonEnabled(true);
#endif
}

QuickModAddFileDialog::~QuickModAddFileDialog()
{
	delete ui;
}

void QuickModAddFileDialog::run(QWidget *parent)
{
	QuickModAddFileDialog dialog(parent);
	if (dialog.exec() == QDialog::Accepted)
	{
		if (dialog.ui->fileButton->isChecked())
		{
			for (auto filename : dialog.ui->fileEdit->text().split(';'))
			{
				MMC->quickmodslist()->registerMod(filename, false);
			}
		}
		else
		{
			MMC->quickmodslist()->registerMod(Util::expandQMURL(dialog.ui->urlEdit->text()), false);
		}
	}
}

void QuickModAddFileDialog::on_addButton_clicked()
{
	if ((ui->fileButton->isChecked() && ui->fileEdit->hasAcceptableInput()) ||
		(ui->urlButton->isChecked() && ui->urlEdit->hasAcceptableInput()))
	{
		accept();
	}
	else
	{
		reject();
	}
}
void QuickModAddFileDialog::on_browseButton_clicked()
{
	if (!ui->fileButton->isChecked())
	{
		return;
	}

	QStringList fileNames =
		QFileDialog::getOpenFileNames(this, tr("Add QuickMod File"), ui->fileEdit->text(),
									  tr("QuickMod files (*.quickmod *.json)"));
	if (!fileNames.isEmpty())
	{
		ui->fileEdit->setText(fileNames.join(';'));
	}
}

void QuickModAddFileDialog::on_fileEdit_textChanged(const QString &)
{
	if (ui->fileButton->isChecked())
	{
		QString input = ui->fileEdit->text();
		int pos = ui->fileEdit->cursorPosition();
		ui->addButton->setEnabled(ui->fileEdit->validator()->validate(input, pos) ==
								  QValidator::Acceptable);
		ui->fileEdit->setText(input);
		ui->fileEdit->setCursorPosition(pos);
	}
}
void QuickModAddFileDialog::on_urlEdit_textChanged(const QString &)
{
	if (ui->urlButton->isChecked())
	{
		ui->addButton->setEnabled(ui->urlEdit->hasAcceptableInput());
	}
}
void QuickModAddFileDialog::on_fileButton_clicked()
{
	ui->addButton->setEnabled(ui->fileEdit->hasAcceptableInput());
}
void QuickModAddFileDialog::on_urlButton_clicked()
{
	ui->addButton->setEnabled(ui->urlEdit->hasAcceptableInput());
}

#include "QuickModAddFileDialog.moc"
