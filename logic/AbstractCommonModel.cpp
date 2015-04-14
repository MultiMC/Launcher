// Licensed under the Apache-2.0 license. See README.md for details.

#include "AbstractCommonModel.h"

CommonModel::CommonModel(const Qt::Orientation orientation, BaseAbstractCommonModel *backend, QObject *parent)
	: QAbstractListModel(parent), m_orientation(orientation), m_backend(backend)
{
}

int CommonModel::rowCount(const QModelIndex &parent) const
{
	return m_orientation == Qt::Horizontal ? m_backend->entryCount() : m_backend->size();
}
int CommonModel::columnCount(const QModelIndex &parent) const
{
	return m_orientation == Qt::Horizontal ? m_backend->size() : m_backend->entryCount();
}
QVariant CommonModel::data(const QModelIndex &index, int role) const
{
	const int i = m_orientation == Qt::Horizontal ? index.column() : index.row();
	const int entry = m_orientation == Qt::Horizontal ? index.row() : index.column();
	return m_backend->formatData(i, role, m_backend->get(i, entry, role));
}
QVariant CommonModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != m_orientation && role == Qt::DisplayRole)
	{
		return m_backend->entryTitle(section);
	}
	else
	{
		return QVariant();
	}
}
bool CommonModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	const int i = m_orientation == Qt::Horizontal ? index.column() : index.row();
	const int entry = m_orientation == Qt::Horizontal ? index.row() : index.column();
	const bool result = m_backend->set(i, entry, role, m_backend->sanetizeData(i, role, value));
	if (result)
	{
		emit dataChanged(index, index, QVector<int>() << role);
	}
	return result;
}
Qt::ItemFlags CommonModel::flags(const QModelIndex &index) const
{
	const int entry = m_orientation == Qt::Horizontal ? index.row() : index.column();
	if (m_backend->canSet(entry))
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	}
	else
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}
}

void CommonModel::notifyAboutToAddObject(const int at)
{
	if (m_orientation == Qt::Horizontal)
	{
		beginInsertColumns(QModelIndex(), at, at);
	}
	else
	{
		beginInsertRows(QModelIndex(), at, at);
	}
}
void CommonModel::notifyObjectAdded()
{
	if (m_orientation == Qt::Horizontal)
	{
		endInsertColumns();
	}
	else
	{
		endInsertRows();
	}
}
void CommonModel::notifyAboutToRemoveObject(const int at)
{
	if (m_orientation == Qt::Horizontal)
	{
		beginRemoveColumns(QModelIndex(), at, at);
	}
	else
	{
		beginRemoveRows(QModelIndex(), at, at);
	}
}
void CommonModel::notifyObjectRemoved()
{
	if (m_orientation == Qt::Horizontal)
	{
		endRemoveColumns();
	}
	else
	{
		endRemoveRows();
	}
}
