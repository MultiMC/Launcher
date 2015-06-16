#pragma once

#include <QTest>
#include <QSignalSpy>
#include <QEventLoop>

#include "logic/auth/BaseAccount.h"
#include "logic/tasks/Task.h"
#include "application/BuildConfig.h"
#include "tests/TestUtil.h"

static void testRunTask(Task *task)
{
	QEventLoop loop;
	QObject::connect(task, &Task::finished, &loop, &QEventLoop::quit);
	task->start();
	if (task->isRunning())
	{
		loop.exec();
	}
}

class AccountTester : public BaseTest
{
	Q_OBJECT

private slots:
	void init()
	{
		BaseTest::init();
		for (const QString &key : requiredTokens())
		{
			const QString fullKey = "MMC_TEST_" + key;
			if (!qEnvironmentVariableIsSet(fullKey.toLocal8Bit()) || qEnvironmentVariableIsEmpty(fullKey.toLocal8Bit()))
			{
				QSKIP(qPrintable(QString("Required environment variable %1 is not set or empty").arg(fullKey)));
			}
		}
	}

	void test_login()
	{
		BaseAccount *acc = createAccount();
		SessionPtr sess = createSession();

		Task *task = acc->createLoginTask(username(), password(), sess);
		testRunTask(task);
		QVERIFY(task->successful());

		afterLoginChecks(acc, sess);
	}
	void test_check()
	{
		BaseAccount *acc = createAccount();
		SessionPtr sess = createSession();

		Task *loginTask = acc->createLoginTask(username(), password(), sess);
		testRunTask(loginTask);
		QVERIFY(loginTask->successful());

		Task *task = acc->createCheckTask(sess);
		testRunTask(task);
		QVERIFY(task->successful());

		afterCheckChecks(acc, sess);
	}
	void test_logout()
	{
		BaseAccount *acc = createAccount();
		SessionPtr sess = createSession();

		Task *task = acc->createLogoutTask(sess);
		if (task)
		{
			Task *loginTask = acc->createLoginTask(username(), password(), sess);
			testRunTask(loginTask);
			QVERIFY(loginTask->successful());

			testRunTask(task);
			QVERIFY(task->successful());

			afterLogoutChecks(acc, sess);
		}
	}

protected:
	virtual QStringList requiredTokens() const = 0;
	virtual QString username() const = 0;
	virtual QString password() const = 0;
	virtual SessionPtr createSession() const { return nullptr; }
	virtual BaseAccount *createAccount() const = 0;
	virtual void afterLoginChecks(BaseAccount *account, SessionPtr session) {}
	virtual void afterCheckChecks(BaseAccount *account, SessionPtr session) {}
	virtual void afterLogoutChecks(BaseAccount *account, SessionPtr session) {}

	QString getToken(const QString &key) const
	{
		return QString::fromLocal8Bit(qgetenv(("MMC_TEST_" + key).toLocal8Bit()));
	}
};

