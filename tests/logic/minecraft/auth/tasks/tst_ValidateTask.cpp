#include <QTest>

#include "logic/minecraft/auth/tasks/ValidateTask.h"
#include "logic/Json.h"

class ValidateTaskTest : public QObject
{
	Q_OBJECT

	std::shared_ptr<ValidateTask> createTask(const QString &accessToken = "")
	{
		MojangAccount *account = new MojangAccount(new MojangAccountType);
		account->setAccessToken(accessToken);
		MojangAuthSessionPtr sess = std::make_shared<MojangAuthSession>();
		return std::make_shared<ValidateTask>(sess, account);
	}

private slots:
	void test_getRequestContent()
	{
		const QJsonObject content = createTask("asdfasdf")->getRequestContent();
		QCOMPARE(Json::requireString(content, "accessToken"), QString("asdfasdf"));
	}
};

QTEST_GUILESS_MAIN(ValidateTaskTest)

#include "tst_ValidateTask.moc"
