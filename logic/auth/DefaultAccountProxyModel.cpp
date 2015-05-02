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

#include "DefaultAccountProxyModel.h"

#include <QDebug>

#include "AccountModel.h"
#include "BaseAccount.h"
#include "InstanceList.h"

DefaultAccountProxyModel::DefaultAccountProxyModel(InstancePtr instance, QObject *parent)
	: QIdentityProxyModel(parent), m_instance(instance)
{
}

void DefaultAccountProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
	QIdentityProxyModel::setSourceModel(sourceModel);
	m_model = dynamic_cast<AccountModel *>(sourceModel)->shared_from_this();
	connect(m_model.get(), &AccountModel::defaultsChanged, this, [this]() {
		emit dataChanged(index(0, 2),
						 index(rowCount(), 2),
						 QVector<int>() << Qt::DisplayRole << Qt::DecorationRole);
	});
}

int DefaultAccountProxyModel::columnCount(const QModelIndex &parent) const
{
	return 3;
}

QVariant DefaultAccountProxyModel::data(const QModelIndex &proxyIndex, int role) const
{
	if (isDefaultsColumn(proxyIndex.column()))
	{
		BaseAccount *account = m_model->getAccount(mapToSource(proxyIndex.sibling(proxyIndex.row(), 0)));
		if (role == Qt::DisplayRole)
		{
			QStringList text;
			if (m_model->isGlobalDefault(account))
			{
				text.append(tr("Global"));
			}
			if (m_instance && m_model->isInstanceDefaultExplicit(m_instance, account))
			{
				text.append(tr("This"));
			}
			return text.join(", ");
		}
		else if (role == Qt::DecorationRole)
		{
			if (m_model->getAccount(account->type(), m_instance) == account)
			{
				return "icon:status-good";
			}
		}

		return QVariant();
	}
	else
	{
		return QIdentityProxyModel::data(proxyIndex, role);
	}
}
QVariant DefaultAccountProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (isDefaultsColumn(section))
	{
		if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		{
			return tr("Defaults");
		}
		return QVariant();
	}
	else
	{
		return QIdentityProxyModel::headerData(section, orientation, role);
	}
}
Qt::ItemFlags DefaultAccountProxyModel::flags(const QModelIndex &index) const
{
	if (isDefaultsColumn(index.column()))
	{
		return flags(index.sibling(index.row(), 0));
	}
	else
	{
		return QIdentityProxyModel::flags(index);
	}
}
QModelIndex DefaultAccountProxyModel::index(int row, int column, const QModelIndex &parent) const
{
	if (isDefaultsColumn(column))
	{
		return createIndex(row, column);
	}
	else
	{
		return QIdentityProxyModel::index(row, column, parent);
	}
}

QModelIndex DefaultAccountProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
	if (isDefaultsColumn(proxyIndex.column()))
	{
		return QModelIndex();
	}
	else
	{
		return QIdentityProxyModel::mapToSource(proxyIndex);
	}
}

bool DefaultAccountProxyModel::isDefaultsColumn(const int col) const
{
	return col == 2;
}
