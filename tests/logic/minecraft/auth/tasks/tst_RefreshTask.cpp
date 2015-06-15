#include <QTest>

#include "logic/minecraft/auth/tasks/RefreshTask.h"
#include "logic/minecraft/auth/MojangAccount.h"
#include "logic/minecraft/auth/MojangAuthSession.h"
#include "logic/Json.h"

using namespace Json;

class RefreshTaskTest : public QObject
{
	Q_OBJECT

	std::shared_ptr<RefreshTask> createTask(const QString &clientToken = "")
	{
		MojangAccount *account = new MojangAccount(new MojangAccountType);
		account->setClientToken(clientToken);
		account->setAccessToken(QString(clientToken).replace("asdf", "fdsa"));
		MojangAuthSessionPtr sess = std::make_shared<MojangAuthSession>();
		return std::make_shared<RefreshTask>(sess, account);
	}

private slots:
	void test_getRequestContent()
	{
		const QJsonObject content = createTask("asdfasdf")->getRequestContent();
		QCOMPARE(requireString(content, "clientToken"), QString("asdfasdf"));
		QCOMPARE(requireString(content, "accessToken"), QString("fdsafdsa"));
		QCOMPARE(requireBoolean(content, "requestUser"), true);
	}

	// test_processResponse is the same as for AuthenticateTaskTest
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
		task->account()->setProfiles({AccountProfile({"alphabeta", "help", false})});
		task->account()->setCurrentProfile("alphabeta");
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

QTEST_GUILESS_MAIN(RefreshTaskTest)

#include "tst_RefreshTask.moc"
