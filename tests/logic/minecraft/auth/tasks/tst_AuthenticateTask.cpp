#include <QTest>

#include "logic/minecraft/auth/tasks/AuthenticateTask.h"
#include "logic/minecraft/auth/MojangAuthSession.h"
#include "logic/minecraft/auth/MojangAccount.h"
#include "logic/Json.h"

using namespace Json;

class AuthenticateTaskTest : public QObject
{
	Q_OBJECT

	std::shared_ptr<AuthenticateTask> createTask(const QString &clientToken = "")
	{
		MojangAccount *account = new MojangAccount();
		account->setClientToken(clientToken);
		MojangAuthSessionPtr sess = std::make_shared<MojangAuthSession>();
		return std::make_shared<AuthenticateTask>(sess, "IWantTea", "ILoveTrillian", account);
	}

private slots:
	void test_getRequestContent_withClientToken()
	{
		// with client token
		const QJsonObject content = createTask("asdfasdf")->getRequestContent();
		QCOMPARE(requireString(requireObject(content, "agent"), "name"), QString("Minecraft"));
		QCOMPARE(requireInteger(requireObject(content, "agent"), "version"), 1);
		QCOMPARE(requireString(content, "username"), QString("IWantTea"));
		QCOMPARE(requireString(content, "password"), QString("ILoveTrillian"));
		QCOMPARE(requireBoolean(content, "requestUser"), true);
		QCOMPARE(requireString(content, "clientToken"), QString("asdfasdf"));
	}
	void test_getRequestContent_withoutClientToken()
	{
		const QJsonObject content = createTask()->getRequestContent();
		QCOMPARE(requireString(requireObject(content, "agent"), "name"), QString("Minecraft"));
		QCOMPARE(requireInteger(requireObject(content, "agent"), "version"), 1);
		QCOMPARE(requireString(content, "username"), QString("IWantTea"));
		QCOMPARE(requireString(content, "password"), QString("ILoveTrillian"));
		QCOMPARE(requireBoolean(content, "requestUser"), true);
		QVERIFY(!content.contains("clientToken"));
	}

	void test_processResponse_withoutClientToken()
	{
		auto task = createTask("asdfasdf");
		task->processResponse({{"clientToken", ""}});
		QCOMPARE(task->state(), YggdrasilTask::STATE_FAILED_HARD);
		QVERIFY(task->failReason().contains("didn't send a client token"));
	}
	void test_processResponse_withDifferentClientToken()
	{
		auto task = createTask("asdfasdf");
		task->processResponse({{"clientToken", "fdsafdsa"}});
		QCOMPARE(task->state(), YggdrasilTask::STATE_FAILED_HARD);
		QVERIFY(task->failReason().contains("attempted to change the client token"));
	}
	void test_processResponse_withoutAccessToken()
	{
		auto task = createTask("asdfasdf");
		task->processResponse({{"clientToken", "asdfasdf"}});
		QCOMPARE(task->state(), YggdrasilTask::STATE_FAILED_HARD);
		QVERIFY(task->failReason().contains("didn't send an access token"));
	}
	void test_processResponse_valid()
	{
		auto task = createTask("asdfasdf");
		task->processResponse(QJsonObject({
								  {"clientToken", "asdfasdf"},
								  {"accessToken", "deadbeaf"},
								  {"availableProfiles", QJsonArray({QJsonObject({{"id", "alphabeta"},{"name", "help"},{"legacy", false}})})},
								  {"selectedProfile", QJsonObject({{"id", "alphabeta"}})},
								  {"user", QJsonObject({{"id", "alphabeta"},{"properties", QJsonArray()}})}
							  }));
		QCOMPARE(task->state(), YggdrasilTask::STATE_SUCCEEDED);
		QCOMPARE(task->account()->clientToken(), QString("asdfasdf"));
		QCOMPARE(task->account()->accessToken(), QString("deadbeaf"));
		QCOMPARE(task->account()->username(), QString("help"));
		QCOMPARE(task->session()->client_token, QString("asdfasdf"));
		QCOMPARE(task->session()->access_token, QString("deadbeaf"));
		QCOMPARE(task->session()->username, QString("help"));
		QCOMPARE(task->session()->user_type, QString("mojang"));
		QCOMPARE(task->session()->uuid, QString("alphabeta"));
		QCOMPARE(task->session()->u.id, QString("alphabeta"));
		QCOMPARE(task->session()->u.properties.size(), 0);
	}
};

QTEST_GUILESS_MAIN(AuthenticateTaskTest)

#include "tst_AuthenticateTask.moc"
