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

#pragma once

#include "tasks/Task.h"
#include <memory>

class BaseAccount;

using ByteArrayDownloadPtr = std::shared_ptr<class ByteArrayDownload>;

class ImgurBaseTask : public Task
{
	Q_OBJECT
protected:
	explicit ImgurBaseTask(QObject *parent = nullptr);

	virtual QUrl endpoint() const;
	virtual QByteArray payload() const = 0;
	virtual void handleResponse(const QJsonObject &obj) = 0;

private slots:
	void networkFinished();

private:
	void executeTask() override;

	ByteArrayDownloadPtr m_dl;
};

class ImgurAuthenticateTask : public ImgurBaseTask
{
public:
	explicit ImgurAuthenticateTask(const QString &pin, BaseAccount *account, QObject *parent = nullptr);

	QByteArray payload() const override;
	void handleResponse(const QJsonObject &obj) override;

private:
	QString m_pin;
	BaseAccount *m_account;
};
class ImgurValidateTask : public ImgurBaseTask
{
public:
	explicit ImgurValidateTask(BaseAccount *account, QObject *parent = nullptr);

	QByteArray payload() const override;
	void handleResponse(const QJsonObject &obj) override;

private:
	BaseAccount *m_account;
};
