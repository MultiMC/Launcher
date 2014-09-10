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

#include "logic/quickmod/QuickModsList.h"
#include "logic/MMCJson.h"
#include "TestUtil.h"

QDebug operator<<(QDebug dbg, const QuickModVersionRef &version)
{
	return dbg << QString("QuickModVersionRef(%1 %2)").arg(version.mod().toString(), version.toString()).toUtf8().constData();
}

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

	QuickModMetadataPtr createTestingMod()
	{
		auto mod = QuickModMetadataPtr(new QuickModMetadata);
		mod->m_name = "testmodname";
		mod->m_uid = QuickModRef("this.should.be.unique");
		mod->m_repo = "some.testing.repo";
		mod->m_description = "test mod description\nsome more";
		mod->m_urls["website"] = QList<QUrl>() << QUrl("http://test.com/");
		mod->m_urls["icon"] = QList<QUrl>() << QUrl("http://test.com/icon.png");
		mod->m_urls["logo"] = QList<QUrl>() << QUrl("http://test.com/logo.png");
		mod->m_updateUrl = QUrl("http://test.com/testmodname.json");
		mod->m_references = {{QuickModRef("OtherName"),QUrl("http://other.com/othername.json")}, {QuickModRef("Other2Name"),QUrl("https://other2.com/other2name.json")},
							 {QuickModRef("stuff"),QUrl("https://stuff.org/stuff.json")}, {QuickModRef("TheWikipediaMod"),QUrl("ftp://wikipedia.org/thewikipediamod.quickmod")}};
		mod->m_modId = "modid";
		mod->m_categories << "cat" << "grep" << "ls" << "cp";
		mod->m_tags << "tag" << "tictactoe";
		mod->m_license = "WTFPL";
		return mod;
	}
	QuickModVersionPtr createTestingVersion(QuickModMetadataPtr mod, const QuickModDownload::DownloadType type)
	{
		auto version = QuickModVersionPtr(new QuickModVersion(mod));
		version->name_ = version->versionString = "1.42";
		version->m_version = Util::Version(version->name_);
		QuickModDownload download;
		download.url = "http://downloads.com/deadbeaf";
		download.type = type;
		version->downloads.append(download);
		version->forgeVersionFilter = "(9.8.42,)";
		version->compatibleVersions << "1.6.2" << "1.6.4";
		version->dependencies = {{QuickModRef("stuff"), qMakePair(QuickModVersionRef(QuickModRef("stuff"), "1.0.0.0.0"), false)}};
		version->recommendations = {{QuickModRef("OtherName"), QuickModVersionRef(QuickModRef("OtherName"), "1.2.3")}};
		version->sha1 = "a68b86df2f3fff44";
		return version;
	}

	void testParsing_data()
	{
		QTest::addColumn<QByteArray>("input");
		QTest::addColumn<QuickModMetadataPtr>("mod");
		QuickModMetadataPtr mod;
		QuickModVersionPtr version;

		mod = createTestingMod();
		version = createTestingVersion(mod, QuickModDownload::Direct);
		version->installType = QuickModVersion::ForgeMod;
		//mod->m_versions = QList<QuickModVersionPtr>() << version;
		QTest::newRow("basic test, forge mod, direct")
			<< TestsInternal::readFile(QFINDTESTDATA(
				   "data/tst_QuickMod_basic test, forge mod, direct")) << mod;

		mod = createTestingMod();
		version = createTestingVersion(mod, QuickModDownload::Parallel);
		version->installType = QuickModVersion::ForgeCoreMod;
		//mod->m_versions = QList<QuickModVersionPtr>() << version;
		QTest::newRow("basic test, core mod, parallel")
			<< TestsInternal::readFile(QFINDTESTDATA(
				   "data/tst_QuickMod_basic test, core mod, parallel")) << mod;

		mod = createTestingMod();
		version = createTestingVersion(mod, QuickModDownload::Sequential);
		version->installType = QuickModVersion::ConfigPack;
		//mod->m_versions = QList<QuickModVersionPtr>() << version;
		QTest::newRow("basic test, config pack, sequential")
			<< TestsInternal::readFile(QFINDTESTDATA(
				   "data/tst_QuickMod_basic test, config pack, sequential")) << mod;
	}
	void testParsing() noexcept
	{
		QFETCH(QByteArray, input);
		QFETCH(QuickModMetadataPtr, mod);

		QuickModMetadataPtr parsed = QuickModMetadataPtr(new QuickModMetadata);

		try
		{
			parsed->parse(MMCJson::ensureObject(MMCJson::parseDocument(input, "QuickMod")));
		}
		catch (MMCError &e)
		{
			qFatal("%s", e.what());
		}

		QCOMPARE(parsed->m_name, mod->m_name);
		QCOMPARE(parsed->m_uid, mod->m_uid);
		QCOMPARE(parsed->m_repo, mod->m_repo);
		QCOMPARE(parsed->m_description, mod->m_description);
		QCOMPARE(parsed->websiteUrl(), mod->websiteUrl());
		QCOMPARE(parsed->iconUrl(), mod->iconUrl());
		QCOMPARE(parsed->logoUrl(), mod->logoUrl());
		QCOMPARE(parsed->m_updateUrl, mod->m_updateUrl);
		QCOMPARE(parsed->m_references, mod->m_references);
		QCOMPARE(parsed->m_modId, mod->m_modId);
		QCOMPARE(parsed->m_categories, mod->m_categories);
		QCOMPARE(parsed->m_tags, mod->m_tags);
		QCOMPARE(parsed->m_license, mod->m_license);

		QuickModVersionPtr parsedVersion;//FIXME = parsed->versionsInternal().first();
		QuickModVersionPtr version;//FIXME = mod->versionsInternal().first();
		QuickModDownload parsedDownload = parsedVersion->downloads.first();
		QuickModDownload download = version->downloads.first();
		QCOMPARE(parsedDownload.url, download.url);
		QCOMPARE((int)parsedDownload.type, (int)download.type);
		QVERIFY(parsedVersion->valid);
		QCOMPARE(parsedVersion->name(), version->name());
		QCOMPARE(parsedVersion->version(), version->version());
		QCOMPARE(parsedVersion->compatibleVersions, version->compatibleVersions);
		QCOMPARE(parsedVersion->forgeVersionFilter, version->forgeVersionFilter);
		QCOMPARE(parsedVersion->dependencies, version->dependencies);
		QCOMPARE(parsedVersion->recommendations, version->recommendations);
		QCOMPARE(parsedVersion->suggestions, version->suggestions);
		QCOMPARE(parsedVersion->conflicts, version->conflicts);
		QCOMPARE(parsedVersion->provides, version->provides);
		QCOMPARE(parsedVersion->sha1, version->sha1);
		QCOMPARE((int)parsedVersion->installType, (int)version->installType);
		parsedVersion->mod = version->mod; // otherwise the below breaks
		QVERIFY(parsedVersion->operator ==(*version));
	}

	void testFileName_data()
	{
		QTest::addColumn<QUrl>("url");
		QTest::addColumn<QuickModMetadataPtr>("mod");
		QTest::addColumn<QString>("result");

		QTest::newRow("jar") << QUrl("http://downloads.org/filename.jar") << TestsInternal::createMod("SomeMod") << "test_repo.SomeMod.jar";
		QTest::newRow("jar, with version") << QUrl("https://notthewebpageyouarelookingfor.droids/mymod-4.2.jar") << TestsInternal::createMod("MyMod") << "test_repo.MyMod.jar";
	}
	void testFileName()
	{
		QFETCH(QUrl, url);
		QFETCH(QuickModMetadataPtr, mod);
		QFETCH(QString, result);

		QCOMPARE(mod->fileName(url), result);
	}
};

QTEST_GUILESS_MAIN(QuickModTest)

#include "tst_QuickMod.moc"
