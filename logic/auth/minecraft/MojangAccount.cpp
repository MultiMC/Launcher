/* Copyright 2013-2015 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
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

#include "MojangAccount.h"
#include "tasks/RefreshTask.h"
#include "tasks/AuthenticateTask.h"
#include "tasks/ValidateTask.h"

#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegExp>
#include <QStringList>
#include <QJsonDocument>
#include <QDebug>

#include "Json.h"

void MojangAccount::load(const QJsonObject &object)
{
	using namespace Json;

	// The JSON object must at least have a username for it to be valid.
	if (!object.value("username").isString())
	{
		throw Exception("Can't load Mojang account info from JSON object. Username field is "
					   "missing or of the wrong type.");
	}

	BaseAccount::load(object);

	setToken("login_username", ensureString(object, "username"));

	const QJsonArray profileArray = ensureArray(object, "profiles");
	if (profileArray.size() < 1)
	{
		throw Exception("Can't load Mojang account with username \"" + username()
					   + "\". No profiles found.");
	}

	for (const QJsonValue &profileVal : profileArray)
	{
		const QJsonObject profileObject = ensureObject(profileVal);
		const QString id = ensureString(profileObject, "id", "");
		const QString name = ensureString(profileObject, "name", "");
		const bool legacy = ensureBoolean(profileObject, QStringLiteral("legacy"), false);
		if (id.isEmpty() || name.isEmpty())
		{
			qWarning() << "Unable to load a profile because it was missing an ID or a name.";
			continue;
		}
		m_profiles.append({id, name, legacy});
	}

	if (object.value("user").isObject())
	{
		MojangAuthSession::User u;
		const QJsonObject userStructure = ensureObject(object, "user");
		u.id = userStructure.value("id").toString();
		m_user = u;
	}

	// Get the currently selected profile.
	const QString currentProfile = ensureString(object, "activeProfile", "");
	if (!currentProfile.isEmpty())
	{
		setCurrentProfile(currentProfile);
	}
}
QJsonObject MojangAccount::save() const
{
	QJsonObject json = BaseAccount::save();

	QJsonArray profileArray;
	for (AccountProfile profile : m_profiles)
	{
		QJsonObject profileObj;
		profileObj.insert("id", profile.id);
		profileObj.insert("name", profile.name);
		profileObj.insert("legacy", profile.legacy);
		profileArray.append(profileObj);
	}
	json.insert("profiles", profileArray);

	QJsonObject userStructure;
	{
		userStructure.insert("id", m_user.id);
		/*
		QJsonObject userAttrs;
		for(auto key: m_user.properties.keys())
		{
			auto array = QJsonArray::fromStringList(m_user.properties.values(key));
			userAttrs.insert(key, array);
		}
		userStructure.insert("properties", userAttrs);
		*/
	}
	json.insert("user", userStructure);

	if (m_currentProfile != -1)
		json.insert("activeProfile", currentProfile().id);

	return json;
}

QString MojangAccount::avatar() const
{
	if (!hasToken("uuid") || token("uuid").isEmpty())
	{
		return QString();
	}
	return "https://crafatar.com/avatars/" + token("uuid");
}
QString MojangAccount::bigAvatar() const
{
	if (!hasToken("uuid") || token("uuid").isEmpty())
	{
		return QString();
	}
	return "https://crafatar.com/renders/body/" + token("uuid");
}

Task *MojangAccount::createLoginTask(const QString &username, const QString &password, SessionPtr session)
{
	if (accountStatus() == NotVerified && password.isEmpty())
	{
		MojangAuthSessionPtr authSession = std::dynamic_pointer_cast<MojangAuthSession>(session);
		if (authSession)
		{
			authSession->status = MojangAuthSession::RequiresPassword;
			populateSessionFromThis(authSession);
		}
		return nullptr;
	}
	setToken("login_username", username);
	return new AuthenticateTask(std::dynamic_pointer_cast<MojangAuthSession>(session),
								username, password, this);
}
Task *MojangAccount::createCheckTask(SessionPtr session)
{
	return new RefreshTask(std::dynamic_pointer_cast<MojangAuthSession>(session), this);
}
Task *MojangAccount::createLogoutTask(SessionPtr session)
{
	return nullptr; // TODO
}

bool MojangAccount::setCurrentProfile(const QString &profileId)
{
	setToken("uuid", profileId);
	for (int i = 0; i < m_profiles.length(); i++)
	{
		if (m_profiles[i].id == profileId)
		{
			m_currentProfile = i;
			setUsername(m_profiles[i].name);
			changed();
			return true;
		}
	}
	return false;
}

AccountProfile MojangAccount::currentProfile() const
{
	if (m_currentProfile == -1)
		return {};
	return m_profiles[m_currentProfile];
}

AccountStatus MojangAccount::accountStatus() const
{
	if (accessToken().isEmpty())
		return NotVerified;
	else
		return Verified;
}

void MojangAccount::populateSessionFromThis(MojangAuthSessionPtr session)
{
	// the user name. you have to have an user name
	session->username = username();
	// volatile auth token
	session->access_token = accessToken();
	// the semi-permanent client token
	session->client_token = clientToken();
	if (!currentProfile().id.isEmpty())
	{
		// profile name
		session->player_name = currentProfile().name;
		// profile ID
		session->uuid = currentProfile().id;
		// 'legacy' or 'mojang', depending on account type
		session->user_type = currentProfile().legacy ? "legacy" : "mojang";
		if (!session->access_token.isEmpty())
		{
			session->session = "token:" + accessToken() + ":" + profiles()[m_currentProfile].id;
		}
		else
		{
			session->session = "-";
		}
	}
	else
	{
		session->player_name = "Player";
		session->session = "-";
	}
	session->u = user();
}
