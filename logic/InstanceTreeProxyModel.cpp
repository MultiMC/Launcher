#include "InstanceTreeProxyModel.h"

#include <QDebug>

#include "gui/groupview/GroupView.h"

InstanceTreeProxyModel::InstanceTreeProxyModel(QObject *parent)
	: QAbstractProxyModel(parent)
{
}

void InstanceTreeProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
	QAbstractProxyModel::setSourceModel(sourceModel);
	setupGroups();
	connect(sourceModel, &QAbstractItemModel::modelReset, this, &InstanceTreeProxyModel::setupGroups);
	connect(sourceModel, &QAbstractItemModel::rowsInserted, [this](QModelIndex, int, int){setupGroups();});
	connect(sourceModel, &QAbstractItemModel::rowsMoved, [this](QModelIndex, int, int){setupGroups();});
	connect(sourceModel, &QAbstractItemModel::rowsRemoved, [this](QModelIndex, int, int){setupGroups();});
}

QModelIndex InstanceTreeProxyModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid() && m_proxyGroupToName.contains(parent))
	{
		return createIndex(row, column, m_groupIds[m_proxyGroupToName[parent]]);
	}
	else
	{
		return createIndex(row, column);
	}
}
QModelIndex InstanceTreeProxyModel::parent(const QModelIndex &child) const
{
	return m_proxyInstanceToGroup.value(child);
}

int InstanceTreeProxyModel::rowCount(const QModelIndex &parent) const
{
	if (!parent.isValid())
	{
		return m_proxyGroupToName.size();
	}
	else
	{
		return m_proxyInstanceToGroup.keys(parent).size();
	}
}
int InstanceTreeProxyModel::columnCount(const QModelIndex &parent) const
{
	return sourceModel()->columnCount();
}

QVariant InstanceTreeProxyModel::data(const QModelIndex &proxyIndex, int role) const
{
	if (m_proxyGroupToName.contains(proxyIndex))
	{
		if (role == Qt::DisplayRole)
		{
			return m_proxyGroupToName[proxyIndex];
		}
		return QVariant();
	}
	return QAbstractProxyModel::data(proxyIndex, role);
}

Qt::ItemFlags InstanceTreeProxyModel::flags(const QModelIndex &index) const
{
	if (m_proxyGroupToName.contains(index))
	{
		return Qt::ItemIsEnabled;
	}
	else
	{
		return QAbstractProxyModel::flags(index);
	}
}

QModelIndex InstanceTreeProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
	return m_proxyToSource.value(proxyIndex);
}
QModelIndex InstanceTreeProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
	return m_proxyToSource.key(sourceIndex);
}

void InstanceTreeProxyModel::setupGroups()
{
	emit layoutAboutToBeChanged();
	m_proxyToSource.clear();
	m_proxyGroupToName.clear();
	m_proxyInstanceToGroup.clear();
	auto source = sourceModel();
	for (int i = 0; i < source->rowCount(); ++i)
	{
		const QModelIndex sourceIndex = source->index(i, 0);
		const QString groupName = sourceIndex.data(GroupViewRoles::GroupRole).toString();
		if (!m_proxyGroupToName.key(groupName).isValid())
		{
			m_proxyGroupToName.insert(createIndex(m_proxyGroupToName.size(), 0), groupName);
			m_groupIds.insert(groupName, m_groupIds.size() + 1);
		}
		const QModelIndex groupIndex = m_proxyGroupToName.key(groupName);
		const QModelIndex proxyIndex = createIndex(m_proxyInstanceToGroup.keys(groupIndex).size(), 0, m_groupIds[groupName]);
		m_proxyInstanceToGroup.insert(proxyIndex, groupIndex);
		m_proxyToSource.insert(proxyIndex, sourceIndex);
	}
	emit layoutChanged();
}
