#pragma once

#include <QString>
#include <QMultiMap>
#include "auth/BaseSession.h"

class MojangAuthSession : public BaseSession
{
public:
	bool makeOffline(const QString &offline_playername) override;

	QString serializeUserProperties();

	enum Status
	{
		Undetermined,
		RequiresPassword,
		PlayableOffline,
		PlayableOnline
	} status = Undetermined;

	struct User
	{
		QString id;
		QMultiMap<QString, QString> properties;
	} u;

	// client token
	QString client_token;
	// account user name
	QString username;
	// combined session ID
	QString session;
	// volatile auth token
	QString access_token;
	// profile name
	QString player_name;
	// profile ID
	QString uuid;
	// 'legacy' or 'mojang', depending on account type
	QString user_type;
	// Did the auth server reply?
	bool auth_server_online = false;
	// Did the user request online mode?
	bool wants_online = true;

	bool invalidPassword() const override { return status == RequiresPassword; }
	QString defaultPlayerName() const override { return player_name; }
};

using MojangAuthSessionPtr = std::shared_ptr<MojangAuthSession>;
