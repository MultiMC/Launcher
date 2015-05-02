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

#include <QAbstractListModel>
#include <memory>

#include "BaseConfigObject.h"
#include "AbstractCommonModel.h"

class BaseAccount;
class BaseAccountType;
class Container;
class AccountTypesModel;
using InstancePtr = std::shared_ptr<class BaseInstance>;

class AccountModel : public AbstractCommonModel<BaseAccount *>,
		public BaseConfigObject,
		public std::enable_shared_from_this<AccountModel>
{
	Q_OBJECT
public:
	explicit AccountModel();
	~AccountModel();

	void registerType(BaseAccountType *type);
	BaseAccountType *type(const QString &id) const;
	QStringList types() const;

	BaseAccount *getAccount(const QModelIndex &index) const;
	BaseAccount *getAccount(const QString &type, const InstancePtr instance = nullptr) const;

	void setGlobalDefault(BaseAccount *account);
	void setInstanceDefault(InstancePtr instance, BaseAccount *account);
	void unsetDefault(const QString &type, InstancePtr instance = nullptr);
	bool isGlobalDefault(BaseAccount *account) const;
	bool isInstanceDefaultExplicit(InstancePtr instance, BaseAccount *account) const;

	BaseAccount *latest() const { return m_latest; }
	QList<BaseAccount *> accountsForType(const QString &type) const;
	bool hasAny(const QString &type) const;

	QAbstractItemModel *typesModel() const;

signals:
	void listChanged();
	void latestChanged();
	void globalDefaultsChanged();
	void defaultsChanged();

public slots:
	void registerAccount(BaseAccount *account);
	void unregisterAccount(BaseAccount *account);
	void accountChanged(BaseAccount *account);

protected:
	void doLoad(const QByteArray &data) override;
	QByteArray doSave() const override;

private:
	//   type          instance
	QMap<QString, QMap<QString, BaseAccount *>> m_defaults;

	BaseAccount *m_latest = nullptr;

	QMap<QString, BaseAccountType *> m_types;
	AccountTypesModel *m_typesModel;
};
