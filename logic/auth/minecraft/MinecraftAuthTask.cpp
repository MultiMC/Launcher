// Licensed under the Apache-2.0 license. See README.md for details.
/*
#include "MinecraftAuthTask.h"

#include <QNetworkReply>

#include "Json.h"
#include "MinecraftAccount.h"
#include "Env.h"

MinecraftAuthTask::MinecraftAuthTask(const QUrl &endpoint, MinecraftAccount *account, QObject *parent)
	: Task(parent), m_account(account), m_url(QUrl("https://authserver.mojang.com/").resolved(endpoint))
{
}

void MinecraftAuthTask::setData(const QJsonObject &data)
{
	m_clientToken = m_account->hasToken("client_token") ? m_account->token("client_token") : QUuid::createUuid().toString();

	QJsonObject d(data);
	d.insert("clientToken", m_clientToken);
	m_data = Json::toText(d);
}

void MinecraftAuthTask::executeTask()
{
	QNetworkRequest req(m_url);
	req.setHeader(QNetworkRequest::UserAgentHeader, "MultiLaunch (Uncached)");
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	m_reply = ENV.qnam()->post(req, m_data);
	connect(m_reply, &QNetworkReply::finished, this, &MinecraftAuthTask::replyFinished);
	connect(m_reply, &QNetworkReply::downloadProgress, this, &MinecraftAuthTask::replyDownProgress);
	connect(m_reply, &QNetworkReply::uploadProgress, this, &MinecraftAuthTask::replyUpProgress);
}
void MinecraftAuthTask::abort()
{
	m_reply->abort();
}

void MinecraftAuthTask::replyFinished()
{
	const QByteArray data = m_reply->readAll();
	using namespace Json;
	if (m_reply->error() != QNetworkReply::NoError && m_reply->error() <= QNetworkReply::UnknownNetworkError)
	{
		emitFailed(tr("Network Error: %1").arg(m_reply->errorString()));
	}
	else
	{
		bool statusIsSuccess = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200;
		if (emptyIsSuccess() && statusIsSuccess && data.isEmpty())
		{
			emitSucceeded();
		}
		else
		{
			if (data.isEmpty())
			{
				handle(QJsonObject());
			}
			else
			{
				try
				{
					const QJsonObject root = ensureObject(ensureDocument(data));
					if (statusIsSuccess && !root.contains("error"))
					{
						if (ensureString(root, "clientToken") != m_clientToken)
						{
							throw Exception(tr("Server returned invalid or modified clientToken"));
						}
						handle(root);
					}
					else
					{
						emitFailed(ensureString(root, "errorMessage"));
					}
				}
				catch (Exception &e)
				{
					emitFailed(tr("Processing Error: %1").arg(e.message()));
				}
			}
		}
	}
}
void MinecraftAuthTask::replyDownProgress(qint64 current, qint64 max)
{
	m_downCur = current;
	m_downMax = max;
	updateProgress();
}
void MinecraftAuthTask::replyUpProgress(qint64 current, qint64 max)
{
	m_upCur = current;
	m_upMax = max;
	updateProgress();
}

void MinecraftAuthTask::updateProgress()
{
	setProgress((m_downCur+m_upCur) / (m_downMax+m_upMax));
}

MinecraftAuthenticateTask::MinecraftAuthenticateTask(const QString &username, const QString &password, MinecraftAccount *account, QObject *parent)
	: MinecraftAuthTask(QUrl("/authenticate"), account, parent)
{
	QJsonObject agent;
	agent.insert("name", "Minecraft");
	agent.insert("version", 1);

	QJsonObject data;
	data.insert("agent", agent);
	data.insert("username", username);
	data.insert("password", password);
	data.insert("requestUser", true);
	setData(data);
}
// TODO multiple profiles
void MinecraftAuthenticateTask::handle(const QJsonObject &response)
{
	using namespace Json;
	const QString accessToken = ensureString(response, "accessToken");
	const QString clientToken = ensureString(response, "clientToken");
	const QJsonObject profile = ensureObject(response, "selectedProfile");
	const QString playerUuid = ensureString(profile, "id");
	const QString playerName = ensureString(profile, "name");
	const QString playerType = ensureBoolean(profile, QStringLiteral("legacy"), false) ? "legacy" : "mojang";
	m_account->setToken("access_token", accessToken);
	m_account->setToken("player_name", playerName);
	m_account->setToken("uuid", playerUuid);
	m_account->setToken("user_type", playerType);
	m_account->setToken("client_token", clientToken);
	const QJsonObject user = ensureObject(response, "user");
	m_account->setToken("user_properties", toText(user.value("properties").toArray()));
	m_account->setUsername(playerName);
	emitSucceeded();
}

MinecraftValidateTask::MinecraftValidateTask(MinecraftAccount *account, QObject *parent)
	: MinecraftAuthTask(QUrl("/refresh"), account, parent)
{
	QJsonObject data;
	data.insert("accessToken", m_account->token("access_token"));
	data.insert("clientToken", m_account->token("client_token"));
	setData(data);
}
void MinecraftValidateTask::handle(const QJsonObject &response)
{
	using namespace Json;
	const QString accessToken = ensureString(response, "accessToken");
	const QJsonObject profile = ensureObject(response, "selectedProfile");
	const QString playerUuid = ensureString(profile, "id");
	const QString playerName = ensureString(profile, "name");
	m_account->setToken("access_token", accessToken);
	m_account->setToken("session", QString("token:%1:%2").arg(accessToken, playerUuid));
	m_account->setToken("player_name", playerName);
	m_account->setToken("uuid", playerUuid);
	m_account->setUsername(playerName);
	emitSucceeded();
}

MinecraftInvalidateTask::MinecraftInvalidateTask(MinecraftAccount *account, QObject *parent)
	: MinecraftAuthTask(QUrl("/invalidate"), account, parent)
{
	QJsonObject data;
	data.insert("accessToken", m_account->token("access_token"));
	data.insert("clientToken", m_account->token("client_token"));
	setData(data);
}
void MinecraftInvalidateTask::handle(const QJsonObject &response)
{
	emitSucceeded();
}
*/
