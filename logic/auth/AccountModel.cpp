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

#include "AccountModel.h"

#include "Json.h"
#include "BaseInstance.h"
#include "BaseAccount.h"
#include "BaseAccountType.h"
#include "FileSystem.h"
#include "resources/ResourceProxyModel.h"
#include "pathutils.h"

#include "minecraft/auth/MojangAccount.h"

#define ACCOUNT_LIST_FORMAT_VERSION 3

class AccountTypesModel : public AbstractCommonModel<BaseAccountType *>
{
public:
	explicit AccountTypesModel() : AbstractCommonModel<BaseAccountType *>(Qt::Vertical)
	{
		addEntry<QString>(0, Qt::UserRole, &BaseAccountType::id);
		addEntry<QString>(0, Qt::DisplayRole, &BaseAccountType::text);
		addEntry<QString>(0, Qt::DecorationRole, &BaseAccountType::icon);
	}
};

AccountModel::AccountModel()
	: AbstractCommonModel(Qt::Vertical), BaseConfigObject("accounts.json")
{
	m_typesModel = new AccountTypesModel;

	registerType(new MinecraftAccountType);

	addEntry<QString>(0, Qt::DecorationRole, &BaseAccount::avatar);
	addEntry<QString>(0, Qt::DisplayRole, &BaseAccount::username);
	addEntry<QString>(0, ResourceProxyModel::PlaceholderRole, [](BaseAccount *) { return "icon:hourglass"; });
	addEntry<QString>(1, Qt::DecorationRole, [this](BaseAccount *account)
	{
		return m_types.contains(account->type()) ? m_types[account->type()]->icon() : QString();
	});
	addEntry<QString>(1, Qt::DisplayRole, [this](BaseAccount *account)
	{
		return m_types.contains(account->type()) ? m_types[account->type()]->text() : account->type();
	});
	addEntry<QString>(1, Qt::UserRole, &BaseAccount::type);
	addEntry<QString>(1, ResourceProxyModel::PlaceholderRole, [](BaseAccount *) { return "icon:hourglass"; });

	setEntryTitle(0, tr("Username"));
	setEntryTitle(1, tr("Type"));

	connect(this, &AccountModel::modelReset, this, &AccountModel::listChanged);
	connect(this, &AccountModel::rowsInserted, this, &AccountModel::listChanged);
	connect(this, &AccountModel::rowsRemoved, this, &AccountModel::listChanged);
	connect(this, &AccountModel::dataChanged, this, &AccountModel::listChanged);
}
AccountModel::~AccountModel()
{
	saveNow();
	delete m_typesModel;
}

void AccountModel::registerType(BaseAccountType *type)
{
	m_types.insert(type->id(), type);
	m_typesModel->append(type);
}
BaseAccountType *AccountModel::type(const QString &id) const
{
	return m_types.value(id);
}
QStringList AccountModel::types() const
{
	return m_types.keys();
}

BaseAccount *AccountModel::getAccount(const QModelIndex &index) const
{
	return index.isValid() ? get(index.row()) : nullptr;
}
BaseAccount *AccountModel::getAccount(const QString &type, const InstancePtr instance) const
{
	QMap<QString, BaseAccount *> defaults = m_defaults[type];
	// instance default?
	if (instance)
	{
		if (defaults.contains(instance->id()))
		{
			return defaults[instance->id()];
		}
	}
	// global default?
	if (defaults.contains(QString()) && defaults.value(QString()))
	{
		return defaults[QString()];
	}
	return nullptr;
}
void AccountModel::setGlobalDefault(BaseAccount *account)
{
	m_defaults[account->type()][QString()] = account;

	m_latest = account;
	emit latestChanged();
	emit globalDefaultsChanged();
	emit defaultsChanged();

	scheduleSave();
}
void AccountModel::setInstanceDefault(InstancePtr instance, BaseAccount *account)
{
	m_defaults[account->type()][instance->id()] = account;

	m_latest = account;
	emit latestChanged();
	emit defaultsChanged();

	scheduleSave();
}
void AccountModel::unsetDefault(const QString &type, InstancePtr instance)
{
	m_defaults[type].remove(instance ? instance->id() : QString());
	if (!instance)
	{
		emit globalDefaultsChanged();
	}
	emit defaultsChanged();

	scheduleSave();
}

bool AccountModel::isGlobalDefault(BaseAccount *account) const
{
	return getAccount(account->type(), nullptr) == account;
}

bool AccountModel::isInstanceDefaultExplicit(InstancePtr instance, BaseAccount *account) const
{
	return m_defaults[account->type()][instance->id()] == account;
}

QList<BaseAccount *> AccountModel::accountsForType(const QString &type) const
{
	QList<BaseAccount *> out;
	for (BaseAccount *acc : getAll())
	{
		if (acc->type() == type)
		{
			out.append(acc);
		}
	}
	return out;
}

bool AccountModel::hasAny(const QString &type) const
{
	for (const BaseAccount *acc : getAll())
	{
		if (acc->type() == type)
		{
			return true;
		}
	}
	return false;
}

QAbstractItemModel *AccountModel::typesModel() const
{
	return m_typesModel;
}

void AccountModel::registerAccount(BaseAccount *account)
{
	append(account);
	connect(account, &BaseAccount::changed, this, &AccountModel::accountChanged);
	m_latest = account;
	emit latestChanged();

	scheduleSave();
}
void AccountModel::unregisterAccount(BaseAccount *account)
{
	Q_ASSERT(find(account) > -1);
	disconnect(account, &BaseAccount::changed, this, &AccountModel::accountChanged);
	remove(find(account));

	// unset the all defaults that are using this account
	const QList<QString> instanceIds = m_defaults[account->type()].keys(account); // get all instance ids that use this account
	for (const auto instanceId : instanceIds)
	{
		m_defaults[account->type()].remove(instanceId);
	}
	if (m_defaults[account->type()].isEmpty())
	{
		m_defaults.remove(account->type());
	}

	m_latest = nullptr;
	emit latestChanged();

	scheduleSave();
}
void AccountModel::accountChanged()
{
	BaseAccount *account = qobject_cast<BaseAccount *>(sender());
	const int row = find(account);
	emit dataChanged(index(row, 0), index(row, 1));

	m_latest = account;
	emit latestChanged();

	scheduleSave();
}

bool AccountModel::doLoad(const QByteArray &data)
{
	using namespace Json;
	const QJsonObject root = requireObject(requireDocument(data));

	QList<BaseAccount *> accs;
	QMap<QString, QMap<QString, BaseAccount *>> defs;

	const int formatVersion = ensureInteger(root, "formatVersion", 0);
	if (formatVersion == 2) // old, pre-multiauth format
	{
		qDebug() << "Old accounts.json file detected. Before migration:" << requireArray(root, "accounts").size() << "accounts";

		const QString active = ensureString(root, "activeAccount", "");
		for (const QJsonObject &account : requireIsArrayOf<QJsonObject>(root, "accounts"))
		{
			BaseAccount *acc = m_types["minecraft"]->createAccount();
			acc->load(formatVersion, account);
			accs.append(acc);

			if (!active.isEmpty() && !acc->loginUsername().isEmpty() && acc->loginUsername() == active)
			{
				m_defaults[acc->type()][QString()] = acc;
				m_latest = acc;
			}
		}

		qDebug() << "Loaded" << accs.size() << "old accounts";

		// back up the old file
		QFile::copy(fileName(), fileName() + ".backup");

		// resave now so we save using the new format
		saveNow();
	}
	else if (formatVersion != ACCOUNT_LIST_FORMAT_VERSION)
	{
		const QString newName = fileName() + ".old";
		qWarning() << "Format version mismatch when loading account list. Existing one will be renamed to " << newName;
		QFile file(fileName());
		if (!file.rename(newName))
		{
			throw Exception(tr("Unable to move to %1: %2").arg(newName, file.errorString()));
		}
	}
	else
	{
		const auto accounts = requireIsArrayOf<QJsonObject>(root, "accounts");
		for (const auto account : accounts)
		{
			const QString type = requireString(account, "type");
			if (!m_types.contains(type))
			{
				qWarning() << "Unable to load account of type" << type << "(unknown factory)";
			}
			else
			{
				BaseAccount *acc = m_types[type]->createAccount();
				acc->load(formatVersion, account);
				accs.append(acc);
			}
		}

		const auto defaults = requireIsArrayOf<QJsonObject>(root, "defaults");
		for (const auto def : defaults)
	{
		const int index = requireInteger(def, "account");
		if (index >= 0 && index < accs.size())
		{
			defs[requireString(def, "type")][requireString(def, "container")]
					= accs.at(index);
		}
	}
	}

	m_defaults = defs;
	for (BaseAccount *acc : accs)
	{
		connect(acc, &BaseAccount::changed, this, &AccountModel::accountChanged);
	}
	setAll(accs);

	return true;
}
QByteArray AccountModel::doSave() const
{
	using namespace Json;
	QJsonArray accounts;
	for (const auto account : getAll())
	{
		QJsonObject obj = account->save();
		obj.insert("type", account->type());
		accounts.append(obj);
	}
	QJsonArray defaults;
	for (auto it = m_defaults.constBegin(); it != m_defaults.constEnd(); ++it)
	{
		for (auto it2 = it.value().constBegin(); it2 != it.value().constEnd(); ++it2)
		{
			QJsonObject obj;
			obj.insert("type", it.key());
			obj.insert("container", it2.key());
			obj.insert("account", find(it2.value()));
			defaults.append(obj);
		}
	}

	QJsonObject root;
	root.insert("formatVersion", ACCOUNT_LIST_FORMAT_VERSION);
	root.insert("accounts", accounts);
	root.insert("defaults", defaults);
	return toBinary(root);
}
