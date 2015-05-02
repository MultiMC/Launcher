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

#include <tasks/Task.h>

#include <QString>
#include <QJsonObject>
#include <QTimer>
#include <qsslerror.h>

#include "../MojangAccount.h"

class QNetworkReply;

/**
 * A Yggdrasil task is a task that performs an operation on a given mojang account.
 */
class YggdrasilTask : public Task
{
	Q_OBJECT
public:
	explicit YggdrasilTask(MojangAuthSessionPtr session, MojangAccount *account, QObject *parent = 0);

	enum AbortedBy
	{
		BY_NOTHING,
		BY_USER,
		BY_TIMEOUT
	} m_aborted = BY_NOTHING;

	/**
	 * Enum for describing the state of the current task.
	 * Used by the getStateMessage function to determine what the status message should be.
	 */
	enum State
	{
		STATE_CREATED,
		STATE_SENDING_REQUEST,
		STATE_PROCESSING_RESPONSE,
		STATE_FAILED_SOFT, //!< soft failure. this generally means the user auth details haven't been invalidated
		STATE_FAILED_HARD, //!< hard failure. auth is invalid
		STATE_SUCCEEDED
	} m_state = STATE_CREATED;

	bool canAbort() const override { return true; }

protected:

	virtual void executeTask();

	/**
	 * Gets the JSON object that will be sent to the authentication server.
	 * Should be overridden by subclasses.
	 */
	virtual QJsonObject getRequestContent() const = 0;

	/**
	 * Gets the endpoint to POST to.
	 * No leading slash.
	 */
	virtual QString getEndpoint() const = 0;

	/**
	 * Processes the response received from the server.
	 * If an error occurred, this should emit a failed signal and return false.
	 * If Yggdrasil gave an error response, it should call setError() first, and then return false.
	 * Otherwise, it should return true.
	 * Note: If the response from the server was blank, and the HTTP code was 200, this function is called with
	 * an empty QJsonObject.
	 */
	virtual void processResponse(const QJsonObject &responseData) = 0;

	/**
	 * Processes an error response received from the server.
	 * The default implementation will read data from Yggdrasil's standard error response format and set it as this task's Error.
	 * \returns a QString error message that will be passed to emitFailed.
	 */
	virtual void processError(const QJsonObject &responseData);

	/**
	 * Returns the state message for the given state.
	 * Used to set the status message for the task.
	 * Should be overridden by subclasses that want to change messages for a given state.
	 */
	virtual QString getStateMessage() const;

private:
	void finalizeSessionSuccess();
	void finalizeSessionFailure(const QString &reason);
	void fillSessionFromAccount();

protected
slots:
	void processReply();
	void refreshTimers(qint64, qint64);
	void heartbeat();
	void sslErrors(QList<QSslError>);

	void changeState(State newState, QString reason=QString());
public
slots:
	virtual void abort() override;
	void abortByTimeout();
	State state();

protected:
	// FIXME: segfault disaster waiting to happen
	MojangAccount *m_account = nullptr;
	MojangAuthSessionPtr m_session;
	QNetworkReply *m_netReply = nullptr;
	QTimer timeout_keeper;
	QTimer counter;
	int count = 0; // num msec since time reset

	const int timeout_max = 30000;
	const int time_step = 50;
};
