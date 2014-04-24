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

	QuickModInstanceModList(InstancePtr instance, std::shared_ptr<ModList> modList, QObject *parent = 0);

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

public
slots:
	void scheduleModForUpdate(const QModelIndex &index);
	void scheduleModForRemoval(const QModelIndex &index);

	void resetModel() {beginResetModel();endResetModel();}

private
slots:
	void quickmodVersionUpdated();
	void quickmodIconUpdated();

private:
	friend class QuickModInstanceModListProxy;
	InstancePtr m_instance;
	std::shared_ptr<ModList> m_modList;

	QMap<QString, QString> quickmods() const;
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
