// Licensed under the Apache-2.0 license. See README.md for details.

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

class AccountModel : public QAbstractListModel, public BaseConfigObject, public std::enable_shared_from_this<AccountModel>
{
	Q_OBJECT
public:
	explicit AccountModel(QObject *parent = nullptr);
	~AccountModel();

	void registerType(BaseAccountType *type);
	BaseAccountType *type(const QString &id) const;
	QStringList types() const;

	BaseAccount *get(const QModelIndex &index) const;
	BaseAccount *get(const QString &type, const InstancePtr instance = nullptr) const;
	void setGlobalDefault(BaseAccount *account);
	void setInstanceDefault(InstancePtr instance, BaseAccount *account);
	BaseAccount *latest() const { return m_latest; }
	QList<BaseAccount *> accountsForType(const QString &type) const;
	bool hasAny(const QString &type) const;

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	QAbstractItemModel *typesModel() const;

signals:
	void listChanged();
	void latestChanged();
	void globalDefaultsChanged();

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
	QList<BaseAccount *> m_accounts;

	BaseAccount *m_latest = nullptr;

	QMap<QString, BaseAccountType *> m_types;
	AccountTypesModel *m_typesModel;
};
