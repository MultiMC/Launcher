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

#include "BaseAccount.h"

#include "Json.h"
#include "AccountModel.h"

BaseAccount::BaseAccount(QObject *parent)
	: QObject(parent)
{
}

void BaseAccount::setUsername(const QString &username)
{
	m_username = username;
	emit changed();
}

void BaseAccount::setToken(const QString &key, const QString &token)
{
	m_tokens.insert(key, token);
	emit changed();
}

void BaseAccount::load(const int formatVersion, const QJsonObject &obj)
{
	using namespace Json;
	m_username = requireString(obj, "username");
	const QJsonObject tokens = ensureObject(obj, "tokens", QJsonObject());
	m_tokens.clear();
	for (auto it = tokens.constBegin(); it != tokens.constEnd(); ++it)
	{
		m_tokens.insert(it.key(), requireString(it.value()));
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
