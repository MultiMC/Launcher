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

#include "logic/quickmod/QuickModSettings.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/InstanceFactory.h"
#include "logic/minecraft/MinecraftVersionList.h"
#include "logic/BaseInstance.h"
#include "TestUtil.h"

QDebug operator<<(QDebug dbg, const QuickModVersionRef &version)
{
	return dbg << QString("QuickModVersionRef(%1)").arg(version.toString()).toUtf8().constData();
}

class QuickModSettingsTest : public QObject
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
		if (current.exists("instances"))
		{
			QDir(current.absoluteFilePath("instances")).removeRecursively();
		}
		if (current.exists("quickmod"))
		{
			QDir(current.absoluteFilePath("quickmod")).removeRecursively();
		}
	}
	void cleanupTestCase()
	{
		QDir current = QDir::current();

		current.remove("quickmod.cfg");

		if (current.exists("instances"))
		{
			current.cd("instances");
			current.removeRecursively();
			current.cdUp();
		}

		if (current.exists("quickmod"))
		{
			current.cd("quickmod");
			current.removeRecursively();
			current.cdUp();
		}
	}

	QuickModVersionPtr createTestingVersion(QuickModMetadataPtr mod)
	{
		auto version = new QuickModVersion(mod);
		version->name_ = "1.42";
		version->versionString = "1.42";
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
		QTest::addColumn<QVector<QuickModMetadataPtr>>("mods");
		QTest::addColumn<QVector<QuickModVersionPtr>>("versions");
		QTest::addColumn<QVector<QString>>("filenames");

		QuickModMetadataPtr testMod = TestsInternal::createMod("TestMod");
		QuickModMetadataPtr testMod2 = TestsInternal::createMod("TestMod2");
		QuickModMetadataPtr testMod3 = TestsInternal::createMod("TestMod3");
		QTest::newRow("basic test") << (QVector<QuickModMetadataPtr>() << testMod << testMod2
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
		QFETCH(QVector<QuickModMetadataPtr>, mods);
		QFETCH(QVector<QuickModVersionPtr>, versions);
		QFETCH(QVector<QString>, filenames);
		Q_ASSERT(mods.size() == versions.size() && mods.size() == filenames.size());

		QuickModSettings *settings = new QuickModSettings();

		for (int i = 0; i < mods.size(); ++i)
		{
			settings->markModAsExists(mods[i], versions[i], filenames[i]);
		}

		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(settings->isModMarkedAsExists(mods[i], versions[i]), true);
			QCOMPARE(settings->existingModFile(mods[i], versions[i]), filenames[i]);
		}

		delete settings;
		settings = new QuickModSettings();

		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(settings->isModMarkedAsExists(mods[i], versions[i]), true);
			QCOMPARE(settings->existingModFile(mods[i], versions[i]), filenames[i]);
		}

		delete settings;
	}

	void testMarkAsInstalledUninstalled_data()
	{
		QTest::addColumn<QVector<QuickModMetadataPtr>>("mods");
		QTest::addColumn<QVector<QuickModVersionPtr>>("versions");
		QTest::addColumn<InstancePtr>("instance");
		QTest::addColumn<QVector<QString>>("filenames");

		InstancePtr instance;
		InstanceFactory::get().createInstance(
			instance, MMC->minecraftlist()->findVersion("1.5.2"), QDir::current().absoluteFilePath("instances/TestInstance"));
		QuickModMetadataPtr testMod = TestsInternal::createMod("TestMod");
		QuickModMetadataPtr testMod2 = TestsInternal::createMod("TestMod2");
		QuickModMetadataPtr testMod3 = TestsInternal::createMod("TestMod3");
		QTest::newRow("basic test") << (QVector<QuickModMetadataPtr>() << testMod << testMod2
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
		QFETCH(QVector<QuickModMetadataPtr>, mods);
		QFETCH(QVector<QuickModVersionPtr>, versions);
		QFETCH(InstancePtr, instance);
		QFETCH(QVector<QString>, filenames);
		Q_ASSERT(mods.size() == versions.size() && mods.size() == filenames.size());

		QuickModSettings *settings = new QuickModSettings();

		// mark all as installed and check if it worked
		for (int i = 0; i < mods.size(); ++i)
		{
			settings->markModAsInstalled(mods[i]->uid(), versions[i], filenames[i], instance);
		}
		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(settings->isModMarkedAsInstalled(mods[i]->uid(), versions[i], instance), true);
			QCOMPARE(settings->installedModFiles(mods[i]->uid(), instance.get())[versions[i]->version()], filenames[i]);
		}

		// reload
		delete settings;
		settings = new QuickModSettings();
		InstancePtr newInstance;
		InstanceFactory::get().loadInstance(newInstance, instance->instanceRoot());

		// re-check after reloading
		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(settings->isModMarkedAsInstalled(mods[i]->uid(), versions[i], newInstance), true);
			QCOMPARE(settings->installedModFiles(mods[i]->uid(), newInstance.get())[versions[i]->version()], filenames[i]);
		}

		// "uninstall" all of them
		for (int i = 0; i < mods.size(); ++i)
		{
			settings->markModAsUninstalled(mods[i]->uid(), versions[i], newInstance);
		}
		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(settings->isModMarkedAsInstalled(mods[i]->uid(), versions[i], newInstance), false);
			QCOMPARE(settings->installedModFiles(mods[i]->uid(), newInstance.get())[versions[i]->version()], QString());
		}

		// reload again
		delete settings;
		settings = new QuickModSettings();
		newInstance.reset();
		InstanceFactory::get().loadInstance(newInstance, instance->instanceRoot());

		// re-check after reloading
		for (int i = 0; i < mods.size(); ++i)
		{
			QCOMPARE(settings->isModMarkedAsInstalled(mods[i]->uid(), versions[i], newInstance), false);
			QCOMPARE(settings->installedModFiles(mods[i]->uid(), newInstance.get())[versions[i]->version()], QString());
		}

		delete settings;
	}
};

QTEST_GUILESS_MAIN_MULTIMC(QuickModSettingsTest)

#include "tst_QuickModSettings.moc"
