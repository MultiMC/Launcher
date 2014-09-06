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

#include "NewInstanceDialog.h"
#include "ui_NewInstanceDialog.h"

#include <QSortFilterProxyModel>

#include "logic/InstanceFactory.h"
#include "logic/BaseVersion.h"
#include "logic/icons/IconList.h"
#include "logic/minecraft/MinecraftVersionList.h"
#include "logic/tasks/Task.h"
#include "logic/quickmod/QuickMod.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModsList.h"

#include "gui/Platform.h"
#include "MultiMC.h"
#include "VersionSelectDialog.h"
#include "ProgressDialog.h"
#include "IconPickerDialog.h"

#include <QLayout>
#include <QPushButton>

class ModpackSortFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	using QSortFilterProxyModel::QSortFilterProxyModel;

	static QAbstractItemModel *create()
	{
		ModpackSortFilterProxyModel *model = new ModpackSortFilterProxyModel;
		model->setSourceModel(MMC->quickmodslist().get());
		return model;
	}

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
	{
		qDebug() << sourceModel()->index(source_row, 0).data(QuickModsList::UidRole).value<QuickModRef>().userFacing();
		return sourceModel()->index(source_row, 0).data(QuickModsList::CategoriesRole).toStringList().contains("Modpack");
	}
};

NewInstanceDialog::NewInstanceDialog(QWidget *parent)
	: QDialog(parent), ui(new Ui::NewInstanceDialog)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	resize(minimumSizeHint());
	layout()->setSizeConstraint(QLayout::SetFixedSize);
	setSelectedVersion(MMC->minecraftlist()->getLatestStable(), true);
	InstIconKey = "infinity";
	ui->iconButton->setIcon(MMC->icons()->getIcon(InstIconKey));
	ui->quickmodBox->setModel(ModpackSortFilterProxyModel::create());

	connect(ui->quickmodBtn, &QRadioButton::toggled, [this](bool){on_quickmodBox_currentIndexChanged(ui->quickmodBox->currentIndex());});
}

NewInstanceDialog::~NewInstanceDialog()
{
	delete ui;
}

void NewInstanceDialog::updateDialogState()
{
	ui->buttonBox->button(QDialogButtonBox::Ok)
		->setEnabled(!instName().isEmpty() && m_selectedVersion);
}

void NewInstanceDialog::setSelectedVersion(BaseVersionPtr version, bool initial)
{
	m_selectedVersion = version;

	if (m_selectedVersion)
	{
		ui->versionTextBox->setText(version->name());
		if(ui->instNameTextBox->text().isEmpty() && !initial)
		{
			ui->instNameTextBox->setText(version->name());
		}
	}
	else
	{
		ui->versionTextBox->setText("");
	}

	updateDialogState();
}

QString NewInstanceDialog::instName() const
{
	return ui->instNameTextBox->text();
}
QString NewInstanceDialog::iconKey() const
{
	return InstIconKey;
}
BaseVersionPtr NewInstanceDialog::selectedVersion() const
{
	return m_selectedVersion;
}
QuickModRef NewInstanceDialog::fromQuickMod() const
{
	return ui->quickmodBox->currentData(QuickModsList::UidRole).value<QuickModRef>();
}

void NewInstanceDialog::on_btnChangeVersion_clicked()
{
	VersionSelectDialog vselect(MMC->minecraftlist().get(), tr("Change Minecraft version"),
								this);
	vselect.exec();
	if (vselect.result() == QDialog::Accepted)
	{
		BaseVersionPtr version = vselect.selectedVersion();
		if (version)
			setSelectedVersion(version);
	}
}
void NewInstanceDialog::on_iconButton_clicked()
{
	IconPickerDialog dlg(this);
	dlg.exec(InstIconKey);

	if (dlg.result() == QDialog::Accepted)
	{
		InstIconKey = dlg.selectedIconKey;
		ui->iconButton->setIcon(MMC->icons()->getIcon(InstIconKey));
	}
}
void NewInstanceDialog::on_instNameTextBox_textChanged(const QString &arg1)
{
	updateDialogState();
}

void NewInstanceDialog::on_quickmodBox_currentIndexChanged(int newIndex)
{
	if (newIndex == -1 || !ui->quickmodBtn->isChecked())
	{
		return;
	}
	QuickModRef quickmod = ui->quickmodBox->itemData(newIndex, QuickModsList::UidRole).value<QuickModRef>();
	if (ui->instNameTextBox->text().isEmpty())
	{
		ui->instNameTextBox->setText(quickmod.userFacing());
	}
	if (!quickmod.findMod()->icon().isNull())
	{
		if (MMC->icons()->addIcon(quickmod.toString(), quickmod.userFacing(), quickmod.findMod()->icon(), MMCIcon::FileBased))
		{
			InstIconKey = quickmod.toString();
			ui->iconButton->setIcon(MMC->icons()->getIcon(InstIconKey));
		}
	}
	const auto versions = quickmod.findVersions();
	if (!versions.isEmpty())
	{
		setSelectedVersion(MMC->minecraftlist()->findVersion(versions.first().findVersion()->compatibleVersions.first()));
	}
}

#include "NewInstanceDialog.moc"
