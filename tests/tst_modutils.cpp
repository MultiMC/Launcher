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

#include "modutils.h"
#include "TestUtil.h"

class ModUtilsTest : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase()
	{

	}
	void cleanupTestCase()
	{

	}

	void test_versionIsInInterval_data()
	{
		QTest::addColumn<QString>("version");
		QTest::addColumn<QString>("interval");
		QTest::addColumn<bool>("result");

		QTest::newRow("empty, true") << "1.2.3" << "" << true;
		QTest::newRow("one version, true") << "1.2.3" << "1.2.3" << true;
		QTest::newRow("one version, false") << "1.2.3" << "1.2.2" << false;

		QTest::newRow("one version inclusive <-> infinity, true") << "1.2.3" << "[1.2.3,)" << true;
		QTest::newRow("one version exclusive <-> infinity, true") << "1.2.3" << "(1.2.2,)" << true;
		QTest::newRow("one version inclusive <-> infitity, false") << "1.2.3" << "[1.2.4,)" << false;
		QTest::newRow("one version exclusive <-> infinity, false") << "1.2.3" << "(1.2.3,)" << false;

		QTest::newRow("infinity <-> one version inclusive, true") << "1.2.3" << "(,1.2.3]" << true;
		QTest::newRow("infinity <-> one version exclusive, true") << "1.2.3" << "(,1.2.4)" << true;
		QTest::newRow("infinity <-> one version inclusive, false") << "1.2.3" << "(,1.2.2]" << false;
		QTest::newRow("infinity <-> one version exclusive, false") << "1.2.3" << "(,1.2.3)" << false;

		QTest::newRow("inclusive <-> inclusive, true") << "1.2.3" << "[1.2.2,1.2.3]" << true;
		QTest::newRow("inclusive <-> exclusive, true") << "1.2.3" << "[1.2.3,1.2.4)" << true;
		QTest::newRow("exclusive <-> inclusive, true") << "1.2.3" << "(1.2.2,1.2.3]" << true;
		QTest::newRow("exclusive <-> exclusive, true") << "1.2.3" << "(1.2.2,1.2.4)" << true;
		QTest::newRow("inclusive <-> inclusive, false") << "1.2.3" << "[1.0.0,1.2.2]" << false;
		QTest::newRow("inclusive <-> exclusive, false") << "1.2.3" << "[1.0.0,1.2.3)" << false;
		QTest::newRow("exclusive <-> inclusive, false") << "1.2.3" << "(1.2.3,2.0.0]" << false;
		QTest::newRow("exclusive <-> exclusive, false") << "1.2.3" << "(1.0.0,1.2.3)" << false;
	}
	void test_versionIsInInterval()
	{
		QFETCH(QString, version);
		QFETCH(QString, interval);
		QFETCH(bool, result);

		QCOMPARE(Util::versionIsInInterval(version, interval), result);
	}

	void test_expandQMURL_data()
	{
		QTest::addColumn<QString>("in");
		QTest::addColumn<QString>("out");

		QTest::newRow("github, default branch, root dir")
				<< "github://MultiMC@MultiMC5/CMakeLists.txt"
				<< "https://raw.github.com/MultiMC/MultiMC5/master/CMakeLists.txt";
		QTest::newRow("github, default branch, not root")
				<< "github://02JanDal@QuickModDoc/_layout/index.html"
				<< "https://raw.github.com/02JanDal/QuickModDoc/master/_layout/index.html";
		QTest::newRow("github, develop branch, root dir")
				<< "github://MultiMC@MultiMC5/CMakeLists.txt#develop"
				<< "https://raw.github.com/MultiMC/MultiMC5/develop/CMakeLists.txt";
		QTest::newRow("github, develop branch, not root")
				<< "github://02JanDal@QuickModDoc/_layout/index.html#gh-pages"
				<< "https://raw.github.com/02JanDal/QuickModDoc/gh-pages/_layout/index.html";

		QTest::newRow("mcf")
				<< "mcf:123456"
				<< "http://www.minecraftforum.net/topic/123456-";
	}
	void test_expandQMURL()
	{
		QFETCH(QString, in);
		QFETCH(QString, out);
		QUrl outUrl(out);

		QCOMPARE(Util::expandQMURL(in), outUrl);
	}
};

QTEST_GUILESS_MAIN_MULTIMC(ModUtilsTest)

#include "tst_modutils.moc"
