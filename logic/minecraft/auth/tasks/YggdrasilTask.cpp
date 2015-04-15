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

#include "YggdrasilTask.h"

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QByteArray>
#include <QDebug>

#include "Env.h"
#include "../MojangAccount.h"
#include "net/URLConstants.h"
#include "Json.h"

YggdrasilTask::YggdrasilTask(MojangAuthSessionPtr session, MojangAccount *account, QObject *parent)
	: Task(parent), m_account(account), m_session(session)
{
	changeState(STATE_CREATED);
}

void YggdrasilTask::executeTask()
{
	changeState(STATE_SENDING_REQUEST);

	QNetworkRequest netRequest(QUrl("https://" + URLConstants::AUTH_BASE + getEndpoint()));
	netRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	m_netReply = ENV.qnam()->post(netRequest, Json::toText(getRequestContent()));
	connect(m_netReply, &QNetworkReply::finished, this, &YggdrasilTask::processReply);
	connect(m_netReply, &QNetworkReply::uploadProgress, this, &YggdrasilTask::refreshTimers);
	connect(m_netReply, &QNetworkReply::downloadProgress, this, &YggdrasilTask::refreshTimers);
	connect(m_netReply, &QNetworkReply::sslErrors, this, &YggdrasilTask::sslErrors);
	timeout_keeper.setSingleShot(true);
	timeout_keeper.start(timeout_max);
	counter.setSingleShot(false);
	counter.start(time_step);
	progress(0, timeout_max);
	connect(&timeout_keeper, &QTimer::timeout, this, &YggdrasilTask::abortByTimeout);
	connect(&counter, &QTimer::timeout, this, &YggdrasilTask::heartbeat);
}

void YggdrasilTask::refreshTimers(qint64, qint64)
{
	timeout_keeper.stop();
	timeout_keeper.start(timeout_max);
	progress(count = 0, timeout_max);
}
void YggdrasilTask::heartbeat()
{
	count += time_step;
	progress(count, timeout_max);
}

void YggdrasilTask::abort()
{
	progress(timeout_max, timeout_max);
	// TODO: actually use this in a meaningful way
	m_aborted = YggdrasilTask::BY_USER;
	m_netReply->abort();
}

void YggdrasilTask::abortByTimeout()
{
	progress(timeout_max, timeout_max);
	// TODO: actually use this in a meaningful way
	m_aborted = YggdrasilTask::BY_TIMEOUT;
	m_netReply->abort();
}

void YggdrasilTask::sslErrors(QList<QSslError> errors)
{
	int i = 1;
	for (auto error : errors)
	{
		qCritical() << "LOGIN SSL Error #" << i << " : " << error.errorString();
		auto cert = error.certificate();
		qCritical() << "Certificate in question:\n" << cert.toText();
		i++;
	}
}

void YggdrasilTask::processReply()
{
	changeState(STATE_PROCESSING_RESPONSE);

	switch (m_netReply->error())
	{
	case QNetworkReply::NoError:
		break;
	case QNetworkReply::TimeoutError:
		changeState(STATE_FAILED_SOFT, tr("Authentication operation timed out."));
		return;
	case QNetworkReply::OperationCanceledError:
		changeState(STATE_FAILED_SOFT, tr("Authentication operation cancelled."));
		return;
	case QNetworkReply::SslHandshakeFailedError:
		changeState(
			STATE_FAILED_SOFT,
			tr("<b>SSL Handshake failed.</b><br/>There might be a few causes for it:<br/>"
			   "<ul>"
			   "<li>You use Windows XP and need to <a "
			   "href=\"http://www.microsoft.com/en-us/download/details.aspx?id=38918\">update "
			   "your root certificates</a></li>"
			   "<li>Some device on your network is interfering with SSL traffic. In that case, "
			   "you have bigger worries than Minecraft not starting.</li>"
			   "<li>Possibly something else. Check the MultiMC log file for details</li>"
			   "</ul>"));
		return;
	// used for invalid credentials and similar errors. Fall through.
	case QNetworkReply::ContentOperationNotPermittedError:
		break;
	default:
		changeState(STATE_FAILED_SOFT,
					tr("Authentication operation failed due to a network error: %1 (%2)")
						.arg(m_netReply->errorString()).arg(m_netReply->error()));
		return;
	}

	const QByteArray replyData = m_netReply->readAll();
	const int responseCode = m_netReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (replyData.isEmpty() && responseCode == 200)
	{
		// an empty reply is also valid
		processResponse(QJsonObject());
		return;
	}

	try
	{
		const QJsonObject response = Json::ensureObject(Json::ensureDocument(replyData));

		if (responseCode == 200)
		{
			processResponse(response);
		}
		else
		{
			// We were able to parse the server's response. Woo!
			// Call processError. If a subclass has overridden it then they'll handle their
			// stuff there.
			qDebug() << "The request failed, but the server gave us an error message. "
						"Processing error.";
			processError(response);
		}
	}
	catch (Exception &e)
	{
		if (responseCode == 200)
		{
			changeState(STATE_FAILED_SOFT, tr("Failed to parse authentication server response: %1").arg(e.cause()));
			qCritical() << replyData;
		}
		else
		{
			// The server didn't say anything regarding the error. Give the user an unknown
			// error.
			qDebug()
				<< "The request failed and the server gave no error message. Unknown error.";
			changeState(STATE_FAILED_SOFT,
						tr("An unknown error occurred when trying to communicate with the "
						   "authentication server: %1").arg(m_netReply->errorString()));
		}
	}
}

void YggdrasilTask::processError(const QJsonObject &responseData)
{
	using namespace Json;

	try
	{
		changeState(STATE_FAILED_HARD, ensureString(responseData, "errorMessage"));
	}
	catch (...)
	{
		// Error is not in standard format. Return unknown error.
		changeState(STATE_FAILED_HARD, tr("An unknown Yggdrasil error occurred."));
	}
}

QString YggdrasilTask::getStateMessage() const
{
	switch (m_state)
	{
	case STATE_CREATED:
		return "Waiting...";
	case STATE_SENDING_REQUEST:
		return tr("Sending request to auth servers...");
	case STATE_PROCESSING_RESPONSE:
		return tr("Processing response from servers...");
	case STATE_SUCCEEDED:
		return tr("Authentication task succeeded.");
	case STATE_FAILED_SOFT:
		return tr("Failed to contact the authentication server.");
	case STATE_FAILED_HARD:
		return tr("Failed to authenticate.");
	default:
		return tr("...");
	}
}

void YggdrasilTask::finalizeSessionSuccess()
{
	if (m_session)
	{
		m_session->status =
			m_session->wants_online ? MojangAuthSession::PlayableOnline : MojangAuthSession::PlayableOffline;
		m_account->populateSessionFromThis(m_session);
		m_session->auth_server_online = true;
	}
}
void YggdrasilTask::finalizeSessionFailure(const QString &reason)
{
	if (state() == YggdrasilTask::STATE_FAILED_SOFT)
	{
		if (m_session)
		{
			m_session->status = m_account->accountStatus() == Verified ? MojangAuthSession::PlayableOffline
														  : MojangAuthSession::RequiresPassword;
			m_session->auth_server_online = false;
			m_account->populateSessionFromThis(m_session);
		}
	}
	else
	{
		m_account->setAccessToken(QString());
		if (m_session)
		{
			m_session->status = MojangAuthSession::RequiresPassword;
			m_session->auth_server_online = true;
			m_account->populateSessionFromThis(m_session);
		}
	}
}

void YggdrasilTask::changeState(YggdrasilTask::State newState, QString reason)
{
	m_state = newState;
	setStatus(getStateMessage());
	if (newState == STATE_SUCCEEDED)
	{
		finalizeSessionSuccess();
		emitSucceeded();
	}
	else if (newState == STATE_FAILED_HARD || newState == STATE_FAILED_SOFT)
	{
		finalizeSessionFailure(reason);
		emitFailed(reason);
	}
}

YggdrasilTask::State YggdrasilTask::state()
{
	return m_state;
}
