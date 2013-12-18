#include <QTest>

#include "gui/dialogs/ChooseQuickModVersionDialog.h"
#include "TestUtil.h"

class ChooseQuickModVersionDialogTest : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase()
	{

	}
	void cleanupTestCase()
	{

	}

	void test_versionIsInFilter_data()
	{
		QTest::addColumn<QString>("version");
		QTest::addColumn<QString>("filter");
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
	void test_versionIsInFilter()
	{
		QFETCH(QString, version);
		QFETCH(QString, filter);
		QFETCH(bool, result);

		QCOMPARE(ChooseQuickModVersionDialog::versionIsInFilter(version, filter), result);
	}
};

QTEST_GUILESS_MAIN_MULTIMC(ChooseQuickModVersionDialogTest)

#include "tst_ChooseQuickModVersionDialog.moc"
