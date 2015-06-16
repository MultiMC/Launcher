#include <QTest>

#include "logic/screenshots/auth/ImgurAccount.h"
#include "AccountTester.h"

class ImgurAccountTest : public AccountTester
{
	Q_OBJECT
private slots:

protected:
	QStringList requiredTokens() const override
	{
		return {"IMGUR_USERNAME", "IMGUR_PASSWORD"};
	}
	QString username() const override
	{
		return QString();
	}
	QString password() const override
	{
		return QString();
	}
	BaseAccount *createAccount() const override
	{
		return new ImgurAccount(new ImgurAccountType);
	}
	void afterLoginChecks(BaseAccount *account, SessionPtr session) override
	{
	}
	void afterCheckChecks(BaseAccount *account, SessionPtr session) override
	{
	}
	void afterLogoutChecks(BaseAccount *account, SessionPtr session) override
	{
	}
};

QTEST_GUILESS_MAIN(ImgurAccountTest)

#include "tst_ImgurAccount.moc"
