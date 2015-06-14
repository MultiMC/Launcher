#include "ModelTester.h"

#include <QDir>
#include <QTemporaryDir>

#include "logic/auth/AccountModel.h"
#include "logic/auth/BaseAccountType.h"
#include "logic/auth/BaseAccount.h"
#include "logic/minecraft/auth/MojangAccount.h"
#include "TestUtil.h"
#include "logic/FileSystem.h"
#include "logic/Json.h"

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
		m->registerAccount(m->type("mojang")->createAccount());
	}

private slots:
	void test_Migrate_V2_to_V3()
	{
		auto checkModel = [](AccountModel *model)
		{
			QCOMPARE(model->size(), 2);
			QVERIFY(model->hasAny("mojang"));
			QCOMPARE(model->accountsForType("mojang").size(), 2);

			MojangAccount *first = dynamic_cast<MojangAccount *>(model->accountsForType("mojang").first());
			QVERIFY(first);
			QCOMPARE(first->username(), QString("IWantTea"));
			QCOMPARE(first->loginUsername(), QString("arthur.philip@dent.co.uk"));
			QCOMPARE(first->clientToken(), QString("f11bc5a96e8428cae87df606c6ed05cb"));
			QCOMPARE(first->accessToken(), QString("214c57e4fe0b58253e3409cdd5e63053"));
			QCOMPARE(first->profiles().size(), 1);
			QCOMPARE(first->profiles().first().id, QString("d716718a0ede7865c8a4a00e9cb1b6f5"));
			QCOMPARE(first->profiles().first().legacy, false);
			QCOMPARE(first->profiles().first().name, QString("IWantTea"));

			MojangAccount *second = dynamic_cast<MojangAccount *>(model->accountsForType("mojang").at(1));
			QVERIFY(second);
			QCOMPARE(second->username(), QString("IAmTheBest"));
			QCOMPARE(second->loginUsername(), QString("zaphod.beeblebrox@galaxy.gov"));
			QCOMPARE(second->clientToken(), QString("d03a2bcf2d1cc467042c7b2680ba947d"));
			QCOMPARE(second->accessToken(), QString("204fe2edcee69f8c207c392e6cc25c9c"));
			QCOMPARE(second->profiles().size(), 1);
			QCOMPARE(second->profiles().first().id, QString("40db0352edab1d1afb8443a34680ef10"));
			QCOMPARE(second->profiles().first().legacy, false);
			QCOMPARE(second->profiles().first().name, QString("IAmTheBest"));
		};

		QFile::copy(QFINDTESTDATA("tests/data/accounts_v2.json"), "accounts.json");
		std::shared_ptr<AccountModel> model = std::make_shared<AccountModel>();
		QVERIFY(model->loadNow());
		model->saveNow();
		checkModel(model.get()); // compare individual fields
		QVERIFY(TestsInternal::compareFiles(QFINDTESTDATA("tests/data/accounts_v3.json"), "accounts.json")); // compare full files
		model.reset(new AccountModel);
		QVERIFY(TestsInternal::compareFiles(QFINDTESTDATA("tests/data/accounts_v2.json"), "accounts.json.backup")); // ensure the backup is created

		QVERIFY(model->loadNow());
		checkModel(model.get());
	}

	void test_Types()
	{
		std::shared_ptr<AccountModel> model = std::dynamic_pointer_cast<AccountModel>(createModel());
		QVERIFY(model->typesModel());
		QVERIFY(!model->types().isEmpty());
		QVERIFY(model->type("mojang"));
		QCOMPARE(model->type("mojang")->id(), QStringLiteral("mojang"));
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
		QCOMPARE(model->getAccount("mojang"), accNull);
		QCOMPARE(model->getAccount("mojang", instance1), accNull);
		QCOMPARE(model->getAccount("mojang", instance2), accNull);
		QCOMPARE(model->getAccount("asdf"), accNull);
		QCOMPARE(model->getAccount("asdf", instance1), accNull);
		QCOMPARE(model->getAccount("asdf", instance2), accNull);

		model->setInstanceDefault(instance1, acc1);
		// instance default
		QCOMPARE(model->getAccount("mojang"), accNull);
		QCOMPARE(model->getAccount("mojang", instance1), acc1);
		QCOMPARE(model->getAccount("mojang", instance2), accNull);
		QCOMPARE(model->getAccount("asdf"), accNull);
		QCOMPARE(model->getAccount("asdf", instance1), accNull);
		QCOMPARE(model->getAccount("asdf", instance2), accNull);

		model->setGlobalDefault(acc2);
		// global default
		QCOMPARE(model->getAccount("mojang"), acc2);
		QCOMPARE(model->getAccount("mojang", instance1), acc1);
		QCOMPARE(model->getAccount("mojang", instance2), acc2);
		QCOMPARE(model->getAccount("asdf"), accNull);
		QCOMPARE(model->getAccount("asdf", instance1), accNull);
		QCOMPARE(model->getAccount("asdf", instance2), accNull);

		model->unsetDefault("mojang");
		// unsetting global default
		QCOMPARE(model->getAccount("mojang"), accNull);
		QCOMPARE(model->getAccount("mojang", instance1), acc1);
		QCOMPARE(model->getAccount("mojang", instance2), accNull);
		QCOMPARE(model->getAccount("asdf"), accNull);
		QCOMPARE(model->getAccount("asdf", instance1), accNull);
		QCOMPARE(model->getAccount("asdf", instance2), accNull);

		model->unsetDefault("mojang", instance1);
		// unsetting instance default
		QCOMPARE(model->getAccount("mojang"), accNull);
		QCOMPARE(model->getAccount("mojang", instance1), accNull);
		QCOMPARE(model->getAccount("mojang", instance2), accNull);
		QCOMPARE(model->getAccount("asdf"), accNull);
		QCOMPARE(model->getAccount("asdf", instance1), accNull);
		QCOMPARE(model->getAccount("asdf", instance2), accNull);
	}
};

QTEST_GUILESS_MAIN(AccountModelTest)

#include "tst_AccountModel.moc"
