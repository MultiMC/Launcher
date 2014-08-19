/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <QTest>
#include <QDir>

#include "logic/quickmod/QuickModsList.h"
#include "logic/InstanceFactory.h"
#include "logic/minecraft/MinecraftVersionList.h"
#include "logic/BaseInstance.h"
#include "TestUtil.h"

QDebug operator<<(QDebug dbg, const QuickModVersionRef &version)
{
	return dbg << QString("QuickModVersionRef(%1)").arg(version.toString()).toUtf8().constData();
}

class QuickModsListTest : public QObject
{
	Q_OBJECT
private
slots:
	void initTestCase()
	{
		Q_INIT_RESOURCE(instances);
		Q_INIT_RESOURCE(multimc);
		Q_INIT_RESOURCE(backgrounds);
		Q_INIT_RESOURCE(versions);

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

	QuickModVersionPtr createTestingVersion(QuickModPtr mod)
	{
		auto version = new QuickModVersion(mod);
		version->name_ = "1.42";
		version->version_ = "1.42";
		version->m_version = Util::Version("1.42");
		QuickModDownload download;
		download.url = "http://downloads.com/deadbeaf";
		version->downloads.append(download);
		version->forgeVersionFilter = "(9.8.42,)";
		version->compatibleVersions << "1.6.2"
									<< "1.6.4";
		version->dependencies = {{QuickModRef("stuff"), qMakePair(QuickModVersionRef(QuickModRef("stuff"), "1.0.0.0.0"), false)}};
		version->recommendations = {{QuickModRef("OtherName"), QuickModVersionRef(QuickModRef("OtherName"), "1.2.3")}};
		version->sha1 = "a68b86df2f3fff44";
		return QuickModVersionPtr(version);
	}

	void testMarkAsExisting_data()
	{
		QTest::addColumn<QVector<QuickModPtr>>("mods");
		QTest::addColumn<QVector<QuickModVersionPtr>>("versions");
		QTest::addColumn<QVector<QString>>("filenames");

		QuickModPtr testMod = TestsInternal::createMod("TestMod");
		QuickModPtr testMod2 = TestsInternal::createMod("TestMod2");
		QuickModPtr testMod3 = TestsInternal::createMod("TestMod3");
		QTest::newRow("basic test") << (QVector<QuickModPtr>() << testMod << testMod2
															  << testMod3)
									<< (QVector<QuickModVersionPtr>()
										<< createTestingVersion(testMod)
										<< createTestingVersion(testMod2)
										<< createTestingVersion(testMod3))
									<< (QVector<QString>()
										<< QDir::current().absoluteFilePath("TestMod.jar")
										<< QDir::current().absoluteFilePath("TestMod2.jar")
										<< QDir::current().absoluteFilePath("TestMod3.jar"));
	}
	void testMarkAsExisting()
	{
		QFETCH(QVector<QuickModPtr>, mods);
		QFETCH(QVector<QuickModVersionPtr>, versions);
		QFETCH(QVector<QString>, filenames);
		Q_ASSERT(mods.size() == versions.size() && mods.size() == filenames.size());

		QuickModsList *list = new QuickModsList(QuickModsList::DontCleanup);

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
		list = new QuickModsList(QuickModsList::DontCleanup);

		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(list->isModMarkedAsExists(mods[i], versions[i]), true);
			QCOMPARE(list->existingModFile(mods[i], versions[i]), filenames[i]);
		}

		delete list;
	}

	void testMarkAsInstalledUninstalled_data()
	{
		QTest::addColumn<QVector<QuickModPtr>>("mods");
		QTest::addColumn<QVector<QuickModVersionPtr>>("versions");
		QTest::addColumn<InstancePtr>("instance");
		QTest::addColumn<QVector<QString>>("filenames");

		InstancePtr instance;
		InstanceFactory::get().createInstance(
			instance, MMC->minecraftlist()->findVersion("1.6.4"), QDir::current().absoluteFilePath("instances/TestInstance"));
		QuickModPtr testMod = TestsInternal::createMod("TestMod");
		QuickModPtr testMod2 = TestsInternal::createMod("TestMod2");
		QuickModPtr testMod3 = TestsInternal::createMod("TestMod3");
		QTest::newRow("basic test") << (QVector<QuickModPtr>() << testMod << testMod2
															  << testMod3)
									<< (QVector<QuickModVersionPtr>()
										<< createTestingVersion(testMod)
										<< createTestingVersion(testMod2)
										<< createTestingVersion(testMod3))
									<< instance
									<< (QVector<QString>()
										<< QDir::current().absoluteFilePath("TestMod.jar")
										<< QDir::current().absoluteFilePath("TestMod2.jar")
										<< QDir::current().absoluteFilePath("TestMod3.jar"));
	}
	void testMarkAsInstalledUninstalled()
	{
		QFETCH(QVector<QuickModPtr>, mods);
		QFETCH(QVector<QuickModVersionPtr>, versions);
		QFETCH(InstancePtr, instance);
		QFETCH(QVector<QString>, filenames);
		Q_ASSERT(mods.size() == versions.size() && mods.size() == filenames.size());

		QuickModsList *list = new QuickModsList(QuickModsList::DontCleanup);

		// mark all as installed and check if it worked
		for (int i = 0; i < mods.size(); ++i)
		{
			list->markModAsInstalled(mods[i]->uid(), versions[i], filenames[i], instance);
		}
		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(list->isModMarkedAsInstalled(mods[i]->uid(), versions[i], instance), true);
			QCOMPARE(list->installedModFiles(mods[i]->uid(), instance.get())[versions[i]->version()], filenames[i]);
		}

		// reload
		delete list;
		list = new QuickModsList(QuickModsList::DontCleanup);
		InstancePtr newInstance;
		InstanceFactory::get().loadInstance(newInstance, instance->instanceRoot());

		// re-check after reloading
		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(list->isModMarkedAsInstalled(mods[i]->uid(), versions[i], newInstance), true);
			QCOMPARE(list->installedModFiles(mods[i]->uid(), newInstance.get())[versions[i]->version()], filenames[i]);
		}

		// "uninstall" all of them
		for (int i = 0; i < mods.size(); ++i)
		{
			list->markModAsUninstalled(mods[i]->uid(), versions[i], newInstance);
		}
		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(list->isModMarkedAsInstalled(mods[i]->uid(), versions[i], newInstance), false);
			QCOMPARE(list->installedModFiles(mods[i]->uid(), newInstance.get())[versions[i]->version()], QString());
		}

		// reload again
		delete list;
		list = new QuickModsList(QuickModsList::DontCleanup);
		newInstance.reset();
		InstanceFactory::get().loadInstance(newInstance, instance->instanceRoot());

		// re-check after reloading
		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(list->isModMarkedAsInstalled(mods[i]->uid(), versions[i], newInstance), false);
			QCOMPARE(list->installedModFiles(mods[i]->uid(), newInstance.get())[versions[i]->version()], QString());
		}

		delete list;
	}
};

QTEST_GUILESS_MAIN_MULTIMC(QuickModsListTest)

#include "tst_QuickModsList.moc"
