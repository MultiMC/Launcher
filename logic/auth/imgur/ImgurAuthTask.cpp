// Licensed under the Apache-2.0 license. See README.md for details.

#include "ImgurAuthTask.h"

#include "auth/BaseAccount.h"
#include "Json.h"

ImgurAuthenticateTask::ImgurAuthenticateTask(const QString &pin, BaseAccount *account, QObject *parent)
	: StandardTask(parent), m_pin(pin), m_account(account)
{
}
void ImgurAuthenticateTask::start()
{
	using namespace Json;
	const QByteArray data = networkPost(QUrl("https://api.imgur.com/oauth2/token"), QString("client_id=%1&client_secret=%2&grant_type=pin&pin=%3")
										.arg(ML_IMGUR_CLIENT_ID,
											 ML_IMGUR_CLIENT_SECRET,
											 m_pin).toLatin1(), true);
	const QJsonObject parsed = ensureObject(ensureDocument(data));
	// error response: {"data":{"error":"Invalid Pin","request":"\/oauth2\/token","method":"POST"},"success":false,"status":400}
	// success response: {"access_token":"<token>","expires_in":3600,"token_type":"bearer","scope":null,"refresh_token":"<token>","account_id":<id>,
	// "account_username":"<username>"}
	if (parsed.contains("success") && !ensureBoolean(parsed, QString("success")))
	{
		const QJsonObject d = ensureObject(parsed, "data");
		if (d.contains("error"))
		{
			setFailure(ensureString(d, "error"));
		}
	}
	const QString accessToken = ensureString(parsed, "access_token");
	const QString refreshToken = ensureString(parsed, "refresh_token");
	const QString username = ensureString(parsed, "account_username");
	m_account->setUsername(username);
	m_account->setToken("accessToken", accessToken);
	m_account->setToken("refreshToken", refreshToken);
	setSuccess();
}

ImgurValidateTask::ImgurValidateTask(BaseAccount *account, QObject *parent)
	: StandardTask(parent), m_account(account)
{
}
void ImgurValidateTask::start()
{
	using namespace Json;
	const QByteArray data = networkPost(QUrl("https://api.imgur.com/oauth2/token"), QString("client_id=%1&client_secret=%2&grant_type=refresh_token&pin=%3")
										.arg(ML_IMGUR_CLIENT_ID,
											 ML_IMGUR_CLIENT_SECRET,
											 m_account->token("refreshToken")).toLatin1(), true);
	const QJsonObject parsed = ensureObject(ensureDocument(data));
	if (parsed.contains("success") && !ensureBoolean(parsed, QString("success")))
	{
		const QJsonObject d = ensureObject(parsed, "data");
		if (d.contains("error"))
		{
			setFailure(ensureString(d, "error"));
		}
	}
	const QString accessToken = ensureString(parsed, "access_token");
	const QString refreshToken = ensureString(parsed, "refresh_token");
	const QString username = ensureString(parsed, "account_username");
	m_account->setUsername(username);
	m_account->setToken("accessToken", accessToken);
	m_account->setToken("refreshToken", refreshToken);
	setSuccess();
}
