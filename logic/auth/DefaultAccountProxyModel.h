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

#pragma once

#include <QIdentityProxyModel>
#include <memory>

using InstancePtr = std::shared_ptr<class BaseInstance>;

class DefaultAccountProxyModel : public QIdentityProxyModel
{
	Q_OBJECT
public:
	explicit DefaultAccountProxyModel(InstancePtr instance, QObject *parent = nullptr);

	void setSourceModel(QAbstractItemModel *sourceModel) override;

	int columnCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &proxyIndex, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

	QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;

private:
	InstancePtr m_instance;
	std::shared_ptr<class AccountModel> m_model;

	bool isDefaultsColumn(const int col) const;
};
