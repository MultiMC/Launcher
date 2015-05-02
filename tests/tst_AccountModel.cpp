#include "ModelTester.h"

#include <QDir>

#include "auth/AccountModel.h"
#include "auth/BaseAccountType.h"
#include "auth/BaseAccount.h"
#include "TestUtil.h"

class AccountModelTest : public ModelTester
{
	Q_OBJECT
public:
	std::shared_ptr<QAbstractItemModel> createModel() const override
	{
		auto m = std::make_shared<AccountModel>();
		m->setSaveTimeout(INT_MAX);
		return m;
	}
	void populate(std::shared_ptr<QAbstractItemModel> model) const override
	{
		auto m = std::dynamic_pointer_cast<AccountModel>(model);
		m->registerAccount(m->type("minecraft")->createAccount(m));
	}

private slots:
	void test_Types()
	{
		std::shared_ptr<AccountModel> model = std::dynamic_pointer_cast<AccountModel>(createModel());
		QVERIFY(model->typesModel());
		QVERIFY(!model->types().isEmpty());
		QVERIFY(model->type("minecraft"));
		QCOMPARE(model->type("minecraft")->id(), QStringLiteral("minecraft"));
	}

	void test_Querying()
	{
		std::shared_ptr<AccountModel> model = std::dynamic_pointer_cast<AccountModel>(createModel());
		populate(model);

		BaseAccount *account = model->getAccount(model->index(0, 0));
		QVERIFY(account);
		QCOMPARE(model->hasAny(account->type()), true);
		QCOMPARE(model->accountsForType(account->type()), QList<BaseAccount *>() << account);
		QCOMPARE(model->latest(), account);
	}

	void test_Defaults()
	{
		TestsInternal::setupTestingEnv();

		std::shared_ptr<AccountModel> model = std::dynamic_pointer_cast<AccountModel>(createModel());
		populate(model);
		populate(model);

		InstancePtr instance1 = TestsInternal::createInstance();
		InstancePtr instance2 = TestsInternal::createInstance();

		BaseAccount *acc1 = model->getAccount(model->index(0, 0));
		BaseAccount *acc2 = model->getAccount(model->index(1, 0));
		BaseAccount *accNull = nullptr;
		QVERIFY(acc1);
		QVERIFY(acc2);

		// no default set
		QCOMPARE(model->getAccount("minecraft"), accNull);
		QCOMPARE(model->getAccount("minecraft", instance1), accNull);
		QCOMPARE(model->getAccount("minecraft", instance2), accNull);
		QCOMPARE(model->getAccount("asdf"), accNull);
		QCOMPARE(model->getAccount("asdf", instance1), accNull);
		QCOMPARE(model->getAccount("asdf", instance2), accNull);

		model->setInstanceDefault(instance1, acc1);
		// instance default
		QCOMPARE(model->getAccount("minecraft"), accNull);
		QCOMPARE(model->getAccount("minecraft", instance1), acc1);
		QCOMPARE(model->getAccount("minecraft", instance2), accNull);
		QCOMPARE(model->getAccount("asdf"), accNull);
		QCOMPARE(model->getAccount("asdf", instance1), accNull);
		QCOMPARE(model->getAccount("asdf", instance2), accNull);

		model->setGlobalDefault(acc2);
		// global default
		QCOMPARE(model->getAccount("minecraft"), acc2);
		QCOMPARE(model->getAccount("minecraft", instance1), acc1);
		QCOMPARE(model->getAccount("minecraft", instance2), acc2);
		QCOMPARE(model->getAccount("asdf"), accNull);
		QCOMPARE(model->getAccount("asdf", instance1), accNull);
		QCOMPARE(model->getAccount("asdf", instance2), accNull);

		model->unsetDefault("minecraft");
		// unsetting global default
		QCOMPARE(model->getAccount("minecraft"), accNull);
		QCOMPARE(model->getAccount("minecraft", instance1), acc1);
		QCOMPARE(model->getAccount("minecraft", instance2), accNull);
		QCOMPARE(model->getAccount("asdf"), accNull);
		QCOMPARE(model->getAccount("asdf", instance1), accNull);
		QCOMPARE(model->getAccount("asdf", instance2), accNull);

		model->unsetDefault("minecraft", instance1);
		// unsetting instance default
		QCOMPARE(model->getAccount("minecraft"), accNull);
		QCOMPARE(model->getAccount("minecraft", instance1), accNull);
		QCOMPARE(model->getAccount("minecraft", instance2), accNull);
		QCOMPARE(model->getAccount("asdf"), accNull);
		QCOMPARE(model->getAccount("asdf", instance1), accNull);
		QCOMPARE(model->getAccount("asdf", instance2), accNull);
	}
};

QTEST_GUILESS_MAIN(AccountModelTest)

#include "tst_AccountModel.moc"
