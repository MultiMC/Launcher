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

#include "AddQuickModFileDialog.h"
#include "ui_AddQuickModFileDialog.h"

#include <QIcon>
#include <QFileDialog>
#include <QUrl>

#include "MultiMC.h"
#include "gui/Platform.h"

class FileValidator : public QValidator
{
	Q_OBJECT
public:
	FileValidator(QObject *parent = 0) : QValidator(parent)
	{
	}

	State validate(QString &input, int &pos) const
	{
		QFileInfo f(input);
		if (!f.exists())
		{
			return Invalid;
		}
		else if (f.isDir())
		{
			return Intermediate;
		}
		else
		{
			return Acceptable;
		}
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
		return QUrl::fromUserInput(input).isValid() ? Acceptable : Invalid;
	}
};

AddQuickModFileDialog::AddQuickModFileDialog(QWidget *parent)
	: QDialog(parent), ui(new Ui::AddQuickModFileDialog)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);

	ui->fileEdit->setValidator(new FileValidator(this));

	connect(ui->cancelButton, &QPushButton::clicked, this, &AddQuickModFileDialog::reject);
}

AddQuickModFileDialog::~AddQuickModFileDialog()
{
	delete ui;
}

QString AddQuickModFileDialog::fileName() const
{
	return ui->fileEdit->text();
}
QUrl AddQuickModFileDialog::url() const
{
	return QUrl::fromUserInput(ui->urlEdit->text());
}

void AddQuickModFileDialog::on_addButton_clicked()
{
	if ((ui->fileButton->isChecked() && ui->fileEdit->hasAcceptableInput()) ||
		(ui->urlButton->isChecked() && ui->urlEdit->hasAcceptableInput()))
	{
		if (ui->fileButton->isChecked())
		{
			m_type = FileName;
		}
		else
		{
			m_type = Url;
		}
		accept();
	}
	else
	{
		reject();
	}
}

void AddQuickModFileDialog::on_browseButton_clicked()
{
	if (!ui->fileButton->isChecked())
	{
		return;
	}

	// TODO batch opening
	QString fileName =
		QFileDialog::getOpenFileName(this, tr("Add QuickMod File"), ui->fileEdit->text(),
									 tr("QuickMod files (*.quickmod *.json)"));
	if (!fileName.isEmpty())
	{
		ui->fileEdit->setText(fileName);
	}
}

#include "AddQuickModFileDialog.moc"
