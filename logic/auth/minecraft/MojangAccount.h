/* Copyright 2013-2015 MultiMC Contributors
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

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QPair>
#include <QMap>

#include <memory>
#include "MojangAuthSession.h"
#include "auth/BaseAccount.h"
#include "auth/BaseAccountType.h"

class Task;
class YggdrasilTask;
class MojangAccount;

typedef std::shared_ptr<MojangAccount> MojangAccountPtr;
Q_DECLARE_METATYPE(MojangAccountPtr)

/**
 * A profile within someone's Mojang account.
 *
 * Currently, the profile system has not been implemented by Mojang yet,
 * but we might as well add some things for it in MultiMC right now so
 * we don't have to rip the code to pieces to add it later.
 */
struct AccountProfile
{
	QString id;
	QString name;
	bool legacy;
};

enum AccountStatus
{
	NotVerified,
	Verified
};

/**
 * Object that stores information about a certain Mojang account.
 *
 * Said information may include things such as that account's username, client token, and access
 * token if the user chose to stay logged in.
 */
class MojangAccount : public QObject, public BaseAccount
{
	Q_OBJECT
public: /* construction */
	//! Do not copy accounts. ever.
	explicit MojangAccount(const MojangAccount &other, QObject *parent = nullptr) = delete;

	//! Default constructor
	explicit MojangAccount(AccountModelPtr model) : BaseAccount(model) {}

	//! Loads a MojangAccount from the given JSON object.
	void load(const QJsonObject &json);

	//! Saves a MojangAccount to a JSON object and returns it.
	QJsonObject save() const override;

public: /* BaseAccount interface (parts of it) */
	QString avatar() const override;
	QString bigAvatar() const override;
	QString type() const override { return "minecraft"; }
	QString loginUsername() const override { return token("login_username"); }
	Task *createLoginTask(const QString &username, const QString &password, SessionPtr session) override;
	Task *createCheckTask(SessionPtr session) override;
	Task *createLogoutTask(SessionPtr session) override;

public: /* manipulation */
		/**
	 * Sets the currently selected profile to the profile with the given ID string.
	 * If profileId is not in the list of available profiles, the function will simply return
	 * false.
	 */
	bool setCurrentProfile(const QString &profileId);

	void setProfiles(const QList<AccountProfile> &profiles)
	{
		m_profiles = profiles;
		changed();
	}
	void setUser(const MojangAuthSession::User &user)
	{
		m_user = user;
		changed();
	}

	// Used to identify the client - the user can have multiple clients for the same account
	// Think: different launchers, all connecting to the same account/profile
	QString clientToken() const { return token("clientToken"); }
	void setClientToken(const QString &token) { setToken("clientToken", token); }
	// Blank if not logged in.
	QString accessToken() const { return token("accessToken"); }
	void setAccessToken(const QString &token) { setToken("accessToken", token); }

public: /* queries */
	QList<AccountProfile> profiles() const
	{
		return m_profiles;
	}

	const MojangAuthSession::User &user()
	{
		return m_user;
	}

	//! Returns the currently selected profile (if none, returns nullptr)
	AccountProfile currentProfile() const;

	//! Returns whether the account is NotVerified, Verified or Online
	AccountStatus accountStatus() const;

protected: /* variables */
	// Index of the selected profile within the list of available
	// profiles. -1 if nothing is selected.
	int m_currentProfile = -1;

	// List of available profiles.
	QList<AccountProfile> m_profiles;

	// the user structure, whatever it is.
	MojangAuthSession::User m_user;

	// current task we are executing here
	std::shared_ptr<YggdrasilTask> m_currentTask;

private:
	void populateSessionFromThis(MojangAuthSessionPtr session);

public:
	friend class YggdrasilTask;
	friend class AuthenticateTask;
	friend class ValidateTask;
	friend class RefreshTask;
};

class MinecraftAccountType : public BaseAccountType
{
public:
	QString id() const override { return "minecraft"; }
	QString text() const override { return QObject::tr("Minecraft"); }
	QString icon() const override { return "minecraft"; }
	QString usernameText() const override { return QObject::tr("E-Mail/Username:"); }
	QString passwordText() const override { return QObject::tr("Password:"); }
	BaseAccount *createAccount(AccountModelPtr model) override { return new MojangAccount(model); }
	Type type() const override { return UsernamePassword; }
};
