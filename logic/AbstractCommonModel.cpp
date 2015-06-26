/* Copyright 2015 MultiMC Contributors
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

#include "AbstractCommonModel.h"

BaseAbstractCommonModel::BaseAbstractCommonModel(QObject *parent)
	: QAbstractListModel(parent)
{
}

int BaseAbstractCommonModel::rowCount(const QModelIndex &parent) const
{
	return size();
}
int BaseAbstractCommonModel::columnCount(const QModelIndex &parent) const
{
	return entryCount();
}
QVariant BaseAbstractCommonModel::data(const QModelIndex &index, int role) const
{
	if (!hasIndex(index.row(), index.column(), index.parent()))
	{
		return QVariant();
	}
	const int i = index.row();
	const int entry = index.column();
	return formatData(i, role, get(i, entry, role));
}
QVariant BaseAbstractCommonModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		return entryTitle(section);
	}
	else
	{
		return QVariant();
	}
}
bool BaseAbstractCommonModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!hasIndex(index.row(), index.column(), index.parent()))
	{
		return false;
	}

	const int i = index.row();
	const int entry = index.column();

	const bool result = set(i, entry, role, sanetizeData(i, role, value));
	if (result)
	{
		emit dataChanged(index, index, QVector<int>() << role);
	}
	return result;
}
Qt::ItemFlags BaseAbstractCommonModel::flags(const QModelIndex &index) const
{
	if (!hasIndex(index.row(), index.column(), index.parent()))
	{
		return Qt::NoItemFlags;
	}

	if (canSet(index.column()))
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	}
	else
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}
}

void BaseAbstractCommonModel::notifyAboutToAddObject(const int at)
{
	beginInsertRows(QModelIndex(), at, at);
}
void BaseAbstractCommonModel::notifyObjectAdded()
{
	endInsertRows();
}
void BaseAbstractCommonModel::notifyAboutToRemoveObject(const int at)
{
	beginRemoveRows(QModelIndex(), at, at);
}
void BaseAbstractCommonModel::notifyObjectRemoved()
{
	endRemoveRows();
}

void BaseAbstractCommonModel::notifyBeginReset()
{
	beginResetModel();
}
void BaseAbstractCommonModel::notifyEndReset()
{
	endResetModel();
}
