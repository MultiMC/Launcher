#include <QTest>
#include <QDir>

#include "logic/lists/QuickModsList.h"
#include "logic/InstanceFactory.h"
#include "logic/MinecraftVersion.h"
#include "logic/BaseInstance.h"
#include "testutil.h"

Q_DECLARE_METATYPE(BaseInstance*)

class QuickModsListTest : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase()
	{
		QDir current = QDir::current();
		current.remove("quickmod.cfg");
		QDir(current.absoluteFilePath("instances")).removeRecursively();
		QDir(current.absoluteFilePath("quickmod")).removeRecursively();
	}
	void cleanupTestCase()
	{
		QDir current = QDir::current();

		current.remove("quickmod.cfg");

		current.cd("instances");
		current.removeRecursively();
		current.cdUp();

		current.cd("quickmod");
		current.removeRecursively();
		current.cdUp();
	}

	void testMarkAsExisting_data()
	{
		QTest::addColumn<QVector<QuickMod *> >("mods");
		QTest::addColumn<QVector<int> >("versions");
		QTest::addColumn<QVector<QString> >("filenames");

		QTest::newRow("basic test") << (QVector<QuickMod *>() << TestsInternal::createMod("TestMod") << TestsInternal::createMod("TestMod2") << TestsInternal::createMod("TestMod3"))
									<< (QVector<int>() << 0 << 0 << 0)
									<< (QVector<QString>()
										<< QDir::current().absoluteFilePath("TestMod.jar")
										<< QDir::current().absoluteFilePath("TestMod2.jar")
										<< QDir::current().absoluteFilePath("TestMod3.jar"));
	}
	void testMarkAsExisting()
	{
		QFETCH(QVector<QuickMod *>, mods);
		QFETCH(QVector<int>, versions);
		QFETCH(QVector<QString>, filenames);
		Q_ASSERT(mods.size() == versions.size() && mods.size() == filenames.size());

		QuickModsList *list = new QuickModsList;

		for (int i = 0; i < mods.size(); ++i)
		{
			list->markModAsExists(mods[i], versions[i], filenames[i]);
		}

		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(list->isModMarkedAsExists(mods[i], versions[i]), true);
			QCOMPARE(list->existingModFile(mods[i], versions[i]), filenames[i]);
		}

		delete list;
		list = new QuickModsList;

		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(list->isModMarkedAsExists(mods[i], versions[i]), true);
			QCOMPARE(list->existingModFile(mods[i], versions[i]), filenames[i]);
		}

		delete list;
	}

	void testMarkAsInstalledUninstalled_data()
	{
		QTest::addColumn<QVector<QuickMod *> >("mods");
		QTest::addColumn<QVector<int> >("versions");
		QTest::addColumn<BaseInstance *>("instance");
		QTest::addColumn<QVector<QString> >("filenames");

		BaseInstance *instance = NULL;
		std::shared_ptr<MinecraftVersion> version;
		version.reset(new MinecraftVersion);
		version->type = MinecraftVersion::OneSix;
		version->m_name = "1.6.4";
		InstanceFactory::get().createInstance(instance, version, QDir::current().absoluteFilePath("instances/TestInstance"));
		QTest::newRow("basic test") << (QVector<QuickMod *>() << TestsInternal::createMod("TestMod") << TestsInternal::createMod("TestMod2") << TestsInternal::createMod("TestMod3"))
									<< (QVector<int>() << 0 << 0 << 0)
									<< instance
									<< (QVector<QString>()
										<< QDir::current().absoluteFilePath("TestMod.jar")
										<< QDir::current().absoluteFilePath("TestMod2.jar")
										<< QDir::current().absoluteFilePath("TestMod3.jar"));
	}
	void testMarkAsInstalledUninstalled()
	{
		QFETCH(QVector<QuickMod *>, mods);
		QFETCH(QVector<int>, versions);
		QFETCH(BaseInstance *, instance);
		QFETCH(QVector<QString>, filenames);
		Q_ASSERT(mods.size() == versions.size() && mods.size() == filenames.size());

		QuickModsList *list = new QuickModsList;

		// mark all as installed and check if it worked
		for (int i = 0; i < mods.size(); ++i)
		{
			list->markModAsInstalled(mods[i], versions[i], filenames[i], instance);
		}
		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(list->isModMarkedAsInstalled(mods[i], versions[i], instance), true);
			QCOMPARE(list->installedModFile(mods[i], versions[i], instance), filenames[i]);
		}

		// reload
		delete list;
		list = new QuickModsList;
		BaseInstance *newInstance = NULL;
		InstanceFactory::get().loadInstance(newInstance, instance->instanceRoot());

		// re-check after reloading
		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(list->isModMarkedAsInstalled(mods[i], versions[i], newInstance), true);
			QCOMPARE(list->installedModFile(mods[i], versions[i], newInstance), filenames[i]);
		}

		// "uninstall" all of them
		for (int i = 0; i < mods.size(); ++i)
		{
			list->markModAsUninstalled(mods[i], versions[i], newInstance);
		}
		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(list->isModMarkedAsInstalled(mods[i], versions[i], newInstance), false);
			QCOMPARE(list->installedModFile(mods[i], versions[i], newInstance), QString());
		}

		// reload again
		delete list;
		list = new QuickModsList;
		newInstance = NULL;
		InstanceFactory::get().loadInstance(newInstance, instance->instanceRoot());

		// re-check after reloading
		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(list->isModMarkedAsInstalled(mods[i], versions[i], newInstance), false);
			QCOMPARE(list->installedModFile(mods[i], versions[i], newInstance), QString());
		}

		delete list;
	}
};

QTEST_GUILESS_MAIN_MULTIMC(QuickModsListTest)

#include "tst_QuickModsList.moc"
