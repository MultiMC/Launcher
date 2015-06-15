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
#include "BaseAccount.h"

class Container;
class AccountTypesModel;
using InstancePtr = std::shared_ptr<class BaseInstance>;

/** @brief The AccountModel class manages accounts and account types
 *
 * All methods that are specialized with an account type can be called either with a BaseAccountType * as the first
 * parameter or as the BaseAccount subclass as a single template parameter.
 * Example: These two are equivalent:
 *   model->getAccount<MojangAccount>();
 *   model->getAccount(model->type<MojangAccount>());
 *
 * The first calling convention is simpler and shorter to use, while the second is useful if you need to store the type
 * before using it with AccountModel.
 *
 * Internally AccountModel keeps an internal ID of each account type, used for looking up the BaseAccountType * from a
 * BaseAccount subclass type. This internal ID is taken from T::staticMetaObject.className(). This means you ALWAYS
 * need to add the Q_OBJECT class to your BaseAccount subclasses, otherwise the internal ID will be "BaseAccount",
 * which obviously won't work out very well...
 */
class AccountModel : public AbstractCommonModel<BaseAccount *>, public BaseConfigObject
{
	Q_OBJECT

	using AccountFactory = std::function<BaseAccount*()>;

public:
	explicit AccountModel();
	~AccountModel();

	/// Registering an account type makes it loadable, createable etc.
	/// Example: model->registerType<MojangAccountType, MojangAccount>("mojang");
	template<typename TypeClass, typename Class>
	void registerType(const QString &storageId)
	{
		static_assert(std::is_base_of<BaseAccountType, TypeClass>::value, "TypeClass needs to be a subclass of BaseAccountType");
		static_assert(std::is_base_of<BaseAccount, Class>::value, "Class needs to be a subclass of BaseAccount");
		BaseAccountType *type = new TypeClass;
		registerTypeInternal(storageId, internalId<Class>(), type, [type]() { return new Class(type); });
	}

	/// Returns the registered BaseAccountType * object for a given BaseAccount subclass type
	template<typename T>
	BaseAccountType *type() const
	{
		return m_types.value(internalId<T>());
	}

	BaseAccount *getAccount(const QModelIndex &index) const;
	template<typename T>
	BaseAccount *getAccount(const InstancePtr instance = nullptr) const
	{
		return getAccount(type<T>(), instance);
	}
	BaseAccount *getAccount(BaseAccountType *type, const InstancePtr instance = nullptr) const;

	void setGlobalDefault(BaseAccount *account);
	void setInstanceDefault(InstancePtr instance, BaseAccount *account);
	template<typename T>
	void unsetDefault(InstancePtr instance = nullptr) { unsetDefault(type<T>(), instance); }
	void unsetDefault(BaseAccountType *type, InstancePtr instance = nullptr);

	/// Returns true if the given account is the global default
	bool isGlobalDefault(BaseAccount *account) const;
	/// Returns true if the given account is default for the instance without taking the global default into account
	bool isInstanceDefaultExplicit(InstancePtr instance, BaseAccount *account) const;

	/// The latest account is the account that was most recently changed
	BaseAccount *latest() const { return m_latest; }
	template<typename T> QList<BaseAccount *> accountsForType() const { return accountsForType(type<T>()); }
	QList<BaseAccount *> accountsForType(BaseAccountType *type) const;
	template<typename T> bool hasAny() const { return hasAny(type<T>()); }
	bool hasAny(BaseAccountType *type) const;

	QAbstractItemModel *typesModel() const;

	template<typename T>
	BaseAccount *createAccount() { return createAccount(type<T>()); }
	BaseAccount *createAccount(BaseAccountType *type) { return m_accountFactories[type](); }

signals:
	void listChanged();
	void latestChanged();
	void globalDefaultsChanged();
	void defaultsChanged();

public slots:
	void registerAccount(BaseAccount *account);
	void unregisterAccount(BaseAccount *account);

private slots:
	void accountChanged();

protected:
	bool doLoad(const QByteArray &data) override;
	QByteArray doSave() const override;

private:
	//   type                    instance
	QMap<BaseAccountType *, QMap<QString, BaseAccount *>> m_defaults;

	BaseAccount *m_latest = nullptr;

	// used to go from a BaseAccount subclass type (not instance!) to BaseAccountType *
	QMap<QString, BaseAccountType *> m_types;
	// used for creating accounts
	QMap<BaseAccountType *, AccountFactory> m_accountFactories;
	// used for storage
	QMap<BaseAccountType *, QString> m_typeStorageIds;
	AccountTypesModel *m_typesModel;

	void registerTypeInternal(const QString &storageId, const QString &internalId, BaseAccountType *type, AccountFactory factory);

	template<typename T> static QString internalId()
	{
		static_assert(std::is_base_of<BaseAccount, T>::value, "T needs to be a subclass of BaseAccount");
		return T::staticMetaObject.className();
	}
};
