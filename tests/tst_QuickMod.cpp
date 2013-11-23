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

	void testParsing_data()
	{
		QTest::addColumn<QByteArray>("input");
		QTest::addColumn<QuickMod *>("mod");
		QuickMod *mod;

		mod = new QuickMod;
		mod->m_name = "testmodname";
		mod->m_description = "test mod description\nsome more";
		mod->m_websiteUrl = QUrl("http://test.com/");
		mod->m_iconUrl = QUrl("http://test.com/icon.png");
		mod->m_logoUrl = QUrl("http://test.com/logo.png");
		mod->m_updateUrl = QUrl("http://test.com/testmodname.json");
		mod->m_recommendedUrls << QUrl("http://other.com/othername.json") << QUrl("https://other2.com/other2name.json");
		mod->m_dependentUrls << QUrl("https://stuff.org/stuff.json") << QUrl("ftp://wikipedia.org/thewikipediamod.quickmod");
		mod->m_nemName = "nemname";
		mod->m_modId = "modid";
		mod->m_categories << "cat" << "grep" << "ls" << "cp";
		mod->m_tags << "tag" << "tictactoe";
		mod->m_type = QuickMod::ForgeMod;
		mod->m_versions << QuickMod::Version("1.42", QUrl("http://downloads.com/deadbeaf"),
											 QStringList() << "1.6.2" << "1.6.4",
											 QMap<QString, QString>({{"stuff", "1.0.0.0.0"}}));
		mod->m_stub = false;
		QTest::newRow("basic test") << Tests::Internal::readFile(QFINDTESTDATA("data/tst_QuickMod_basic test")) << mod;
	}
	void testParsing()
	{
		QFETCH(QByteArray, input);
		QFETCH(QuickMod *, mod);

		QuickMod *parsed = new QuickMod;
		parsed->m_noFetchImages = true;

		QBENCHMARK { parsed->parse(input, 0); }

		QCOMPARE(parsed->m_name, mod->m_name);
		QCOMPARE(parsed->m_description, mod->m_description);
		QCOMPARE(parsed->m_websiteUrl, mod->m_websiteUrl);
		QCOMPARE(parsed->m_iconUrl, mod->m_iconUrl);
		QCOMPARE(parsed->m_logoUrl, mod->m_logoUrl);
		QCOMPARE(parsed->m_updateUrl, mod->m_updateUrl);
		QCOMPARE(parsed->m_recommendedUrls, mod->m_recommendedUrls);
		QCOMPARE(parsed->m_dependentUrls, mod->m_dependentUrls);
		QCOMPARE(parsed->m_nemName, mod->m_nemName);
		QCOMPARE(parsed->m_modId, mod->m_modId);
		QCOMPARE(parsed->m_categories, mod->m_categories);
		QCOMPARE(parsed->m_tags, mod->m_tags);
		QCOMPARE(parsed->m_type, mod->m_type);
		QCOMPARE(parsed->m_versions, mod->m_versions);
		QCOMPARE(parsed->m_stub, mod->m_stub);
	}
};

QTEST_GUILESS_MAIN(QuickModTest)

#include "tst_QuickMod.moc"
