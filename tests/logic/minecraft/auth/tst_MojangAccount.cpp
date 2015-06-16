#include <QTest>

#include "logic/minecraft/auth/MojangAccount.h"
#include "logic/minecraft/auth/MojangAuthSession.h"
#include "AccountTester.h"

class MojangAccountTest : public AccountTester
{
	Q_OBJECT
private slots:

protected:
	QStringList requiredTokens() const override
	{
		return {"MOJANG_USERNAME", "MOJANG_PASSWORD"};
	}
	QString username() const override
	{
		return getToken("MOJANG_USERNAME");
	}
	QString password() const override
	{
		return getToken("MOJANG_PASSWORD");
	}
	SessionPtr createSession() const override
	{
		return std::make_shared<MojangAuthSession>();
	}
	BaseAccount *createAccount() const override
	{
		return new MojangAccount(new MojangAccountType);
	}
	void afterLoginChecks(BaseAccount *account, SessionPtr session) override
	{
		MojangAccount *acc = dynamic_cast<MojangAccount *>(account);
		MojangAuthSessionPtr sess = std::dynamic_pointer_cast<MojangAuthSession>(session);
		QVERIFY(sess->auth_server_online);
		QCOMPARE(acc->clientToken(), sess->client_token);
		QCOMPARE(acc->accessToken(), sess->access_token);
		QCOMPARE(acc->username(), sess->username);
		QCOMPARE(acc->loginUsername(), username());
		QCOMPARE(acc->profiles().size(), 1);
	}
	void afterCheckChecks(BaseAccount *account, SessionPtr session) override
	{
		afterLoginChecks(account, session);
	}
	void afterLogoutChecks(BaseAccount *account, SessionPtr session) override
	{
	}
};

QTEST_GUILESS_MAIN(MojangAccountTest)

#include "tst_MojangAccount.moc"
