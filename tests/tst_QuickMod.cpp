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
		mod->m_uid = "this.should.be.unique";
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
		return mod;
	}

	void testParsing_data()
	{
		QTest::addColumn<QByteArray>("input");
		QTest::addColumn<QuickModPtr>("mod");
		QuickModPtr mod;

		mod = createTestingMod();
		QTest::newRow("basic test") << TestsInternal::readFile(QFINDTESTDATA("data/tst_QuickMod_basic test")) << mod;
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
