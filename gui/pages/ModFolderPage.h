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

#pragma once

#include <QWidget>

#include "logic/quickmod/QuickModInstanceModList.h"
#include "logic/OneSixInstance.h"
#include "logic/net/NetJob.h"
#include "BasePage.h"

class EnabledItemFilter;
class ModList;

namespace Ui
{
class ModFolderPage;
}

class ModFolderPage : public QWidget, public BasePage
{
	Q_OBJECT

public:
	explicit ModFolderPage(QuickModInstanceModList::Type type, BaseInstance *inst, std::shared_ptr<ModList> mods, QString id,
						   QString iconName, QString displayName, QString helpPage = "",
						   QWidget *parent = 0);
	virtual ~ModFolderPage();
	virtual QString displayName() const override
	{
		return m_displayName;
	}
	virtual QIcon icon() const override
	{
		return QIcon::fromTheme(m_iconName);
	}
	virtual QString id() const override
	{
		return m_id;
	}
	virtual QString helpPage() const override
	{
		return m_helpName;
	}
	virtual bool shouldDisplay() const;

protected:
	bool eventFilter(QObject *obj, QEvent *ev);
	bool modListFilter(QKeyEvent *ev);

protected:
	BaseInstance *m_inst;

	Ui::ModFolderPage *ui;
	std::shared_ptr<ModList> m_mods;
	QString m_iconName;
	QString m_id;
	QString m_displayName;
	QString m_helpName;

public
slots:
	virtual void modCurrent(const QModelIndex &current, const QModelIndex &previous);

private
slots:
	virtual void on_addModBtn_clicked();
	virtual void on_rmModBtn_clicked();
	virtual void on_viewModBtn_clicked();
	virtual void on_updateModBtn_clicked();
	void on_orphansRemoveBtn_clicked();

private:
	QuickModInstanceModList *m_modsModel;
	QuickModInstanceModListProxy *m_proxy;
	QList<QuickModUid> m_orphans;

	void updateOrphans();

	QModelIndex mapToModsList(const QModelIndex &view) const;
	void sortMods(const QModelIndexList &view, QModelIndexList *quickmods, QModelIndexList *mods);
};

class CoreModFolderPage : public ModFolderPage
{
	Q_OBJECT
public:
	explicit CoreModFolderPage(BaseInstance *inst, std::shared_ptr<ModList> mods, QString id,
							   QString iconName, QString displayName, QString helpPage = "",
							   QWidget *parent = 0);
	virtual ~CoreModFolderPage()
	{
	}
	virtual bool shouldDisplay() const;
};
