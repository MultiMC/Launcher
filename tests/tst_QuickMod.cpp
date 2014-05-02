#include <QTest>

#include "logic/quickmod/QuickModsList.h"
#include "TestUtil.h"

class QuickModTest : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase()
	{

	}
	void cleanupTestCase()
	{

	}

	QuickModPtr createTestingMod()
	{
		auto mod = QuickModPtr(new QuickMod);
		mod->m_name = "testmodname";
		mod->m_uid = QuickModUid("this.should.be.unique");
		mod->m_description = "test mod description\nsome more";
		mod->m_websiteUrl = QUrl("http://test.com/");
		mod->m_iconUrl = QUrl("http://test.com/icon.png");
		mod->m_logoUrl = QUrl("http://test.com/logo.png");
		mod->m_updateUrl = QUrl("http://test.com/testmodname.json");
		mod->m_references = {{QuickModUid("OtherName"),QUrl("http://other.com/othername.json")}, {QuickModUid("Other2Name"),QUrl("https://other2.com/other2name.json")},
							 {QuickModUid("stuff"),QUrl("https://stuff.org/stuff.json")}, {QuickModUid("TheWikipediaMod"),QUrl("ftp://wikipedia.org/thewikipediamod.quickmod")}};
		mod->m_nemName = "nemname";
		mod->m_modId = "modid";
		mod->m_categories << "cat" << "grep" << "ls" << "cp";
		mod->m_tags << "tag" << "tictactoe";
		return mod;
	}
	QuickModVersionPtr createTestingVersion()
	{
		auto version = QuickModVersionPtr(new QuickModVersion(TestsInternal::createMod("TheMod")));
		version->name_ = "1.42";
		version->url = QUrl("http://downloads.com/deadbeaf");
		version->forgeVersionFilter = "(9.8.42,)";
		version->compatibleVersions << "1.6.2" << "1.6.4";
		version->dependencies = {{QuickModUid("stuff"), "1.0.0.0.0"}};
		version->recommendations = {{QuickModUid("OtherName"), "1.2.3"}};
		version->md5 = "a68b86df2f3fff44";
		return version;
	}

	void testParsing_data()
	{
		QTest::addColumn<QByteArray>("input");
		QTest::addColumn<QuickModPtr>("mod");
		QuickModPtr mod;
		QuickModVersionPtr version;

		mod = createTestingMod();
		version = createTestingVersion();
		version->downloadType = QuickModVersion::Direct;
		version->installType = QuickModVersion::ForgeMod;
		mod->m_versions = QList<QuickModVersionPtr>() << version;
		QTest::newRow("basic test, forge mod, direct")
			<< TestsInternal::readFile(QFINDTESTDATA(
				   "data/tst_QuickMod_basic test, forge mod, direct")) << mod;

		mod = createTestingMod();
		version = createTestingVersion();
		version->downloadType = QuickModVersion::Parallel;
		version->installType = QuickModVersion::ForgeCoreMod;
		mod->m_versions = QList<QuickModVersionPtr>() << version;
		QTest::newRow("basic test, core mod, parallel")
			<< TestsInternal::readFile(QFINDTESTDATA(
				   "data/tst_QuickMod_basic test, core mod, parallel")) << mod;

		mod = createTestingMod();
		version = createTestingVersion();
		version->downloadType = QuickModVersion::Sequential;
		version->installType = QuickModVersion::ConfigPack;
		mod->m_versions = QList<QuickModVersionPtr>() << version;
		QTest::newRow("basic test, config pack, sequential")
			<< TestsInternal::readFile(QFINDTESTDATA(
				   "data/tst_QuickMod_basic test, config pack, sequential")) << mod;
	}
	void testParsing()
	{
		QFETCH(QByteArray, input);
		QFETCH(QuickModPtr, mod);

		QuickModPtr parsed = QuickModPtr(new QuickMod);

		QBENCHMARK
		{
			try
			{
				parsed->parse(parsed, input);
			}
			catch (MMCError &e)
			{
				qFatal(e.what());
			}
		}

		QCOMPARE(parsed->m_name, mod->m_name);
		QCOMPARE(parsed->m_description, mod->m_description);
		QCOMPARE(parsed->m_websiteUrl, mod->m_websiteUrl);
		QCOMPARE(parsed->m_iconUrl, mod->m_iconUrl);
		QCOMPARE(parsed->m_logoUrl, mod->m_logoUrl);
		QCOMPARE(parsed->m_updateUrl, mod->m_updateUrl);
		QCOMPARE(parsed->m_references, mod->m_references);
		QCOMPARE(parsed->m_nemName, mod->m_nemName);
		QCOMPARE(parsed->m_modId, mod->m_modId);
		QCOMPARE(parsed->m_categories, mod->m_categories);
		QCOMPARE(parsed->m_tags, mod->m_tags);

		QuickModVersionPtr parsedVersion = parsed->versions().first();
		QuickModVersionPtr version = mod->versions().first();
		QVERIFY(parsedVersion->valid);
		QCOMPARE(parsedVersion->name(), version->name());
		QCOMPARE(parsedVersion->url, version->url);
		QCOMPARE(parsedVersion->compatibleVersions, version->compatibleVersions);
		QCOMPARE(parsedVersion->forgeVersionFilter, version->forgeVersionFilter);
		QCOMPARE(parsedVersion->dependencies, version->dependencies);
		QCOMPARE(parsedVersion->recommendations, version->recommendations);
		QCOMPARE(parsedVersion->suggestions, version->suggestions);
		QCOMPARE(parsedVersion->breaks, version->breaks);
		QCOMPARE(parsedVersion->conflicts, version->conflicts);
		QCOMPARE(parsedVersion->provides, version->provides);
		QCOMPARE(parsedVersion->md5, version->md5);
		QCOMPARE((int)parsedVersion->downloadType, (int)version->downloadType);
		QCOMPARE((int)parsedVersion->installType, (int)version->installType);
		parsedVersion->mod = version->mod; // otherwise the below breaks
		QVERIFY(parsedVersion->operator ==(*version));
	}

	void testFileName_data()
	{
		QTest::addColumn<QUrl>("url");
		QTest::addColumn<QuickModPtr>("mod");
		QTest::addColumn<QString>("result");

		QTest::newRow("jar") << QUrl("http://downloads.org/filename.jar") << TestsInternal::createMod("SomeMod") << "SomeMod.jar";
		QTest::newRow("jar, with version") << QUrl("https://notthewebpageyouarelookingfor.droids/mymod-4.2.jar") << TestsInternal::createMod("MyMod") << "MyMod.jar";
	}
	void testFileName()
	{
		QFETCH(QUrl, url);
		QFETCH(QuickModPtr, mod);
		QFETCH(QString, result);

		QCOMPARE(mod->fileName(url), result);
	}
};

QTEST_GUILESS_MAIN_MULTIMC(QuickModTest)

#include "tst_QuickMod.moc"
