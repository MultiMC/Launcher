// Licensed under the Apache-2.0 license. See README.md for details.

#include "BaseAccount.h"

#include "Json.h"
#include "AccountModel.h"

BaseAccount::BaseAccount(AccountModelPtr model)
	: m_model(model)
{
}

void BaseAccount::setUsername(const QString &username)
{
	m_username = username;
	changed();
}

void BaseAccount::setToken(const QString &key, const QString &token)
{
	m_tokens.insert(key, token);
	changed();
}

void BaseAccount::load(const QJsonObject &obj)
{
	using namespace Json;
	m_username = ensureString(obj, "username");
	const QJsonObject tokens = ensureObject(obj, "tokens", QJsonObject());
	m_tokens.clear();
	for (auto it = tokens.constBegin(); it != tokens.constEnd(); ++it)
	{
		m_tokens.insert(it.key(), ensureString(it.value()));
	}
}
QJsonObject BaseAccount::save() const
{
	QJsonObject obj;
	obj.insert("username", m_username);
	QJsonObject tokens;
	for (auto it = m_tokens.constBegin(); it != m_tokens.constEnd(); ++it)
	{
		tokens.insert(it.key(), it.value());
	}
	obj.insert("tokens", tokens);
	return obj;
}

void BaseAccount::changed()
{
	if (auto model = m_model.lock())
	{
		model->accountChanged(this);
	}
}
