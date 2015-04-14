// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once
/*
#include "tasks/Task.h"

#include <QUrl>

class QNetworkReply;
class MinecraftAccount;

class MinecraftAuthTask : public Task
{
	Q_OBJECT
public:
	explicit MinecraftAuthTask(const QUrl &endpoint, MinecraftAccount *account, QObject *parent = nullptr);

	void setData(const QJsonObject &data);

protected:
	virtual void handle(const QJsonObject &response) = 0;
	virtual bool emptyIsSuccess() const { return false; }

	MinecraftAccount *m_account;

private slots:
	void replyFinished();
	void replyDownProgress(qint64 current, qint64 max);
	void replyUpProgress(qint64 current, qint64 max);

private:
	void executeTask() override;
	void abort() override;
	bool canAbort() const override { return true; }

	QUrl m_url;
	QByteArray m_data;
	QString m_clientToken;

	QNetworkReply *m_reply = nullptr;
	qint64 m_downCur = 0;
	qint64 m_downMax = 0;
	qint64 m_upCur = 0;
	qint64 m_upMax = 0;
	void updateProgress();
};

class MinecraftAuthenticateTask : public MinecraftAuthTask
{
	Q_OBJECT
public:
	explicit MinecraftAuthenticateTask(const QString &username, const QString &password, MinecraftAccount *account, QObject *parent = nullptr);

private:
	void handle(const QJsonObject &response) override;
};
class MinecraftValidateTask : public MinecraftAuthTask
{
	Q_OBJECT
public:
	explicit MinecraftValidateTask(MinecraftAccount *account, QObject *parent = nullptr);

private:
	void handle(const QJsonObject &response) override;
};
class MinecraftInvalidateTask : public MinecraftAuthTask
{
	Q_OBJECT
public:
	explicit MinecraftInvalidateTask(MinecraftAccount *account, QObject *parent = nullptr);

private:
	void handle(const QJsonObject &response) override;
	bool emptyIsSuccess() const override { return true; }
};
*/
