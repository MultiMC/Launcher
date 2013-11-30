#include <QTest>

#include "logic/lists/QuickModsList.h"
#include "testutil.h"

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

	QuickMod *createTestingMod()
	{
		auto mod = new QuickMod;
		mod->m_name = "testmodname";
		mod->m_description = "test mod description\nsome more";
		mod->m_websiteUrl = QUrl("http://test.com/");
		mod->m_iconUrl = QUrl("http://test.com/icon.png");
		mod->m_logoUrl = QUrl("http://test.com/logo.png");
		mod->m_updateUrl = QUrl("http://test.com/testmodname.json");
		mod->m_references = {{"OtherName",QUrl("http://other.com/othername.json")}, {"Other2Name",QUrl("https://other2.com/other2name.json")},
							 {"stuff",QUrl("https://stuff.org/stuff.json")}, {"TheWikipediaMod",QUrl("ftp://wikipedia.org/thewikipediamod.quickmod")}};
		mod->m_nemName = "nemname";
		mod->m_modId = "modid";
		mod->m_categories << "cat" << "grep" << "ls" << "cp";
		mod->m_tags << "tag" << "tictactoe";
		mod->m_type = QuickMod::ForgeMod;
		mod->m_versions << QuickMod::Version("1.42", QUrl("http://downloads.com/deadbeaf"),
											 QStringList() << "1.6.2" << "1.6.4",
											 QString("(9.8.42,)"),
											 QMap<QString, QString>({{"stuff", "1.0.0.0.0"}}),
											 QMap<QString, QString>({{"OtherName", "1.2.3"}}),
											 "a68b86df2f3fff44");
		mod->m_stub = false;
		return mod;
	}

	void testParsing_data()
	{
		QTest::addColumn<QByteArray>("input");
		QTest::addColumn<QuickMod *>("mod");
		QuickMod *mod;

		mod = createTestingMod();
		QTest::newRow("basic test, forge mod") << TestsInternal::readFile(QFINDTESTDATA("data/tst_QuickMod_basic test, forge mod")) << mod;

		mod = createTestingMod();
		mod->m_type = QuickMod::ForgeCoreMod;
		QTest::newRow("basic test, core mod") << TestsInternal::readFile(QFINDTESTDATA("data/tst_QuickMod_basic test, core mod")) << mod;

		mod = createTestingMod();
		mod->m_type = QuickMod::ResourcePack;
		QTest::newRow("basic test, resource pack") << TestsInternal::readFile(QFINDTESTDATA("data/tst_QuickMod_basic test, resource pack")) << mod;

		mod = createTestingMod();
		mod->m_type = QuickMod::ConfigPack;
		QTest::newRow("basic test, config pack") << TestsInternal::readFile(QFINDTESTDATA("data/tst_QuickMod_basic test, config pack")) << mod;
	}
	void testParsing()
	{
		QFETCH(QByteArray, input);
		QFETCH(QuickMod *, mod);

		QuickMod *parsed = new QuickMod;
		parsed->m_noFetchImages = true;

		QString errorString;
		QBENCHMARK { parsed->parse(input, &errorString); }
		QCOMPARE(errorString, QString(""));

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
		QCOMPARE(parsed->m_type, mod->m_type);
		QCOMPARE(parsed->m_versions, mod->m_versions);
		QCOMPARE(parsed->m_stub, mod->m_stub);
	}

	void testFileName_data()
	{
		QTest::addColumn<QUrl>("url");
		QTest::addColumn<QuickMod *>("mod");
		QTest::addColumn<QString>("result");

		QTest::newRow("jar") << QUrl("http://downloads.org/filename.jar") << TestsInternal::createMod("SomeMod") << "SomeMod.jar";
		QTest::newRow("jar, with version") << QUrl("https://notthewebpageyouarelookingfor.droids/mymod-4.2.jar") << TestsInternal::createMod("MyMod") << "MyMod.jar";
	}
	void testFileName()
	{
		QFETCH(QUrl, url);
		QFETCH(QuickMod *, mod);
		QFETCH(QString, result);

		QCOMPARE(mod->fileName(url), result);
	}
};

QTEST_GUILESS_MAIN(QuickModTest)

#include "tst_QuickMod.moc"
