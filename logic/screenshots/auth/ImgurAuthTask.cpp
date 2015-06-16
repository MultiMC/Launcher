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

#include "ImgurAuthTask.h"

#include "net/ByteArrayDownload.h"
#include "net/NetJob.h"
#include "auth/BaseAccount.h"
#include "Json.h"
#include "Env.h"

ImgurAuthenticateTask::ImgurAuthenticateTask(const QString &pin, BaseAccount *account, QObject *parent)
	: ImgurBaseTask(parent), m_pin(pin), m_account(account)
{
}

QByteArray ImgurAuthenticateTask::payload() const
{
	return QString("client_id=%1&client_secret=%2&grant_type=pin&pin=%3")
			.arg(ENV.configuration()["ImgurClientID"].toString(),
			ENV.configuration()["ImgurClientSecret"].toString(),
			m_pin).toLatin1();
}

void ImgurAuthenticateTask::handleResponse(const QJsonObject &obj)
{
	using namespace Json;

	// error response: {"data":{"error":"Invalid Pin","request":"\/oauth2\/token","method":"POST"},"success":false,"status":400}
	// success response: {"access_token":"<token>","expires_in":3600,"token_type":"bearer","scope":null,"refresh_token":"<token>","account_id":<id>,
	// "account_username":"<username>"}
	if (!ensureBoolean(obj, QString("success"), true))
	{
		const QJsonObject d = ensureObject(obj, "data");
		if (d.contains("error"))
		{
			throw Exception(requireString(d, "error"));
		}
	}
	const QString accessToken = requireString(obj, "access_token");
	const QString refreshToken = requireString(obj, "refresh_token");
	const QString username = requireString(obj, "account_username");
	m_account->setUsername(username);
	m_account->setToken("accessToken", accessToken);
	m_account->setToken("refreshToken", refreshToken);
}

ImgurValidateTask::ImgurValidateTask(BaseAccount *account, QObject *parent)
	: ImgurBaseTask(parent), m_account(account)
{
}

QByteArray ImgurValidateTask::payload() const
{
	return QString("client_id=%1&client_secret=%2&grant_type=refresh_token&pin=%3")
			.arg(ENV.configuration()["ImgurClientID"].toString(),
			ENV.configuration()["ImgurClientSecret"].toString(),
			m_account->token("refreshToken")).toLatin1();
}

void ImgurValidateTask::handleResponse(const QJsonObject &obj)
{
	using namespace Json;

	if (!ensureBoolean(obj, QString("success"), true))
	{
		const QJsonObject d = requireObject(obj, "data");
		if (d.contains("error"))
		{
			throw Exception(requireString(d, "error"));
		}
	}
	const QString accessToken = requireString(obj, "access_token");
	const QString refreshToken = requireString(obj, "refresh_token");
	const QString username = requireString(obj, "account_username");
	m_account->setUsername(username);
	m_account->setToken("accessToken", accessToken);
	m_account->setToken("refreshToken", refreshToken);
}

ImgurBaseTask::ImgurBaseTask(QObject *parent)
	: Task(parent)
{
}
QUrl ImgurBaseTask::endpoint() const
{
	return QUrl("https://api.imgur.com/oauth2/token");
}
void ImgurBaseTask::executeTask()
{
	m_dl = ByteArrayDownload::make(endpoint());
	m_dl->m_postData = payload();
	NetJob *job = new NetJob("Imgur authentication");
	job->addNetAction(m_dl);
	connect(job, &NetJob::failed, this, &ImgurBaseTask::emitFailed);
	connect(job, &NetJob::progress, this, &ImgurBaseTask::setProgress);
	connect(job, &NetJob::succeeded, this, &ImgurBaseTask::networkFinished);
	connect(job, &NetJob::finished, job, &NetJob::deleteLater);
	job->start();
}
void ImgurBaseTask::networkFinished()
{
	try
	{
		handleResponse(Json::requireObject(Json::requireDocument(m_dl->m_data)));
		emitSucceeded();
	}
	catch (Exception &e)
	{
		emitFailed(e.cause());
	}
}
