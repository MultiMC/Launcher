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

#pragma once

#include <QDialog>

#include "logic/BaseInstance.h"
#include "logic/quickmod/QuickModMetadata.h"
#include "BasePage.h"

class OneSixInstance;

namespace Ui
{
class QuickModBrowsePage;
}

class QItemSelection;
class QListView;
class ModFilterProxyModel;
class CheckboxProxyModel;

class QuickModBrowsePage : public QWidget, public BasePage
{
	Q_OBJECT

public:
	explicit QuickModBrowsePage(std::shared_ptr<OneSixInstance> instance, QWidget *parent = 0);
	~QuickModBrowsePage();

	QString id() const override { return "quickmod-browse"; }
	QString displayName() const override { return tr("QuickMods"); }
	QIcon icon() const override { return QIcon::fromTheme("plugin-red"); }
	bool apply() override;
	bool shouldDisplay() const override;
	void opened() override;

private
slots:
	void on_createInstanceButton_clicked();
	void on_categoriesLabel_linkActivated(const QString &link);
	void on_tagsLabel_linkActivated(const QString &link);
	void on_mcVersionsLabel_linkActivated(const QString &link);
	void on_fulltextEdit_textChanged();
	void on_tagsEdit_textChanged();
	void on_categoryBox_currentTextChanged();
	void on_mcVersionBox_currentTextChanged();
	void on_addButton_clicked();
	void on_updateButton_clicked();
	void on_createFromInstanceBtn_clicked();
	void modSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

	void modLogoUpdated();

	void setupComboBoxes();

	void checkStateChanged(const QModelIndex &index, const bool checked);

private:
	Ui::QuickModBrowsePage *ui;

	QuickModMetadataPtr m_currentMod;

	std::shared_ptr<OneSixInstance> m_instance;

	QListView *m_view;
	ModFilterProxyModel *m_filterModel;
	CheckboxProxyModel *m_checkModel;

	bool m_isSingleSelect = false;
};
