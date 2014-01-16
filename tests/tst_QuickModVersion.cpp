#include <QTest>

#include "logic/quickmod/QuickModVersion.h"
#include "TestUtil.h"

class QuickModVersionTest : public QObject
{
	Q_OBJECT
private
slots:
	void initTestCase()
	{
	}
	void cleanupTestCase()
	{
	}

	QuickModVersion *createTestingVersion()
	{
		auto version = new QuickModVersion(TestsInternal::createMod("TheMod"));
		version->name_ = "1.42";
		version->url = QUrl("http://downloads.com/deadbeaf");
		version->forgeVersionFilter = "(9.8.42,)";
		version->compatibleVersions << "1.6.2" << "1.6.4";
		version->dependencies = {{"stuff", "1.0.0.0.0"}};
		version->recommendations = {{"OtherName", "1.2.3"}};
		version->checksum = "a68b86df2f3fff44";
		version->checksum_algorithm = QCryptographicHash::Md5;
		return version;
	}

	void testParsing_data()
	{
		QTest::addColumn<QByteArray>("input");
		QTest::addColumn<QuickModVersion *>("version");
		QuickModVersion *version;

		version = createTestingVersion();
		version->downloadType = QuickModVersion::Direct;
		version->installType = QuickModVersion::ForgeMod;
		QTest::newRow("basic test, forge mod, direct")
			<< TestsInternal::readFile(QFINDTESTDATA(
				   "data/tst_QuickModVersion_basic test, forge mod, direct")) << version;

		version = createTestingVersion();
		version->downloadType = QuickModVersion::Parallel;
		version->installType = QuickModVersion::ForgeCoreMod;
		QTest::newRow("basic test, core mod, parallel")
			<< TestsInternal::readFile(QFINDTESTDATA(
				   "data/tst_QuickModVersion_basic test, core mod, parallel")) << version;

		version = createTestingVersion();
		version->downloadType = QuickModVersion::Sequential;
		version->installType = QuickModVersion::ConfigPack;
		QTest::newRow("basic test, config pack, sequential")
			<< TestsInternal::readFile(QFINDTESTDATA(
				   "data/tst_QuickModVersion_basic test, config pack, sequential")) << version;
	}
	void testParsing()
	{
		QFETCH(QByteArray, input);
		QFETCH(QuickModVersion *, version);

		QuickModVersion *parsed = new QuickModVersion;
		parsed->mod = version->mod;

		QString errorString;
		QBENCHMARK
		{
			parsed->parse(QJsonDocument::fromJson(input).array().first().toObject(), &errorString);
		}
		QCOMPARE(errorString, QString(""));

		QVERIFY(parsed->valid);
		QCOMPARE(parsed->name(), version->name());
		QCOMPARE(parsed->url, version->url);
		QCOMPARE(parsed->compatibleVersions, version->compatibleVersions);
		QCOMPARE(parsed->forgeVersionFilter, version->forgeVersionFilter);
		QCOMPARE(parsed->dependencies, version->dependencies);
		QCOMPARE(parsed->recommendations, version->recommendations);
		QCOMPARE(parsed->suggestions, version->suggestions);
		QCOMPARE(parsed->breaks, version->breaks);
		QCOMPARE(parsed->conflicts, version->conflicts);
		QCOMPARE(parsed->provides, version->provides);
		QCOMPARE(parsed->checksum_algorithm, version->checksum_algorithm);
		QCOMPARE(parsed->checksum, version->checksum);
		QCOMPARE((int)parsed->downloadType, (int)version->downloadType);
		QCOMPARE((int)parsed->installType, (int)version->installType);
		QVERIFY(parsed->operator ==(*version));
	}
};

QTEST_GUILESS_MAIN_MULTIMC(QuickModVersionTest)

#include "tst_QuickModVersion.moc"
