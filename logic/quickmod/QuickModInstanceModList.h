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

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include "logic/OneSixInstance.h"
#include "QuickModVersion.h"
#include "QuickMod.h"

class QuickModInstanceModList : public QAbstractListModel
{
	Q_OBJECT
public:
	enum Columns
	{
		ActiveColumn = 0,
		NameColumn,
		VersionColumn
	};

	enum Type
	{
		Mods,
		CoreMods,
		ResourcePacks,
		TexturePacks
	};

	QuickModInstanceModList(const Type type, std::shared_ptr<OneSixInstance> instance, std::shared_ptr<ModList> modList,
							QObject *parent = 0);
	~QuickModInstanceModList();

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index = QModelIndex()) const;

	bool setData(const QModelIndex &index, const QVariant &value, int role);
	QMimeData *mimeData(const QModelIndexList &indexes) const;
	QStringList mimeTypes() const;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
					  const QModelIndex &parent);
	Qt::DropActions supportedDragActions() const;
	Qt::DropActions supportedDropActions() const;

	QModelIndex mapToModList(const QModelIndex &index) const;
	QModelIndex mapFromModList(const QModelIndex &index) const;
	bool isModListArea(const QModelIndex &index) const;

	QList<QuickModUid> findOrphans() const;

	Type type() const { return m_type; }

public
slots:
	void updateMods(const QModelIndexList &list);
	void removeMods(const QModelIndexList &list);

	void resetModel()
	{
		beginResetModel();
		endResetModel();
	}

private
slots:
	void quickmodIconUpdated();

private:
	friend class QuickModInstanceModListProxy;
	std::shared_ptr<OneSixInstance> m_instance;
	std::shared_ptr<ModList> m_modList;
	Type m_type;

	QMap<QuickModUid, QString> quickmods() const;
	QuickModPtr modAt(const int row) const;
};

class QuickModInstanceModListProxy : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	QuickModInstanceModListProxy(QuickModInstanceModList *list, QObject *parent = 0);

	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
	QuickModInstanceModList *m_list;
};
