#include <QTest>
#include "TestUtil.h"

#include "logic/settings/INIFile.h"

class IniFileTest : public BaseTest
{
	Q_OBJECT
private
slots:

	void test_Escape_data()
	{
		QTest::addColumn<QString>("through");

		QTest::newRow("unix path") << "/abc/def/ghi/jkl";
		QTest::newRow("windows path") << "C:\\Program files\\terrible\\name\\of something\\";
		QTest::newRow("Plain text") << "Lorem ipsum dolor sit amet.";
		QTest::newRow("Escape sequences") << "Lorem\n\t\n\\n\\tAAZ\nipsum dolor\n\nsit amet.";
		QTest::newRow("Escape sequences 2") << "\"\n\n\"";
	}
	void test_Escape()
	{
		QFETCH(QString, through);

		QString there = INIFile::escape(through);
		QString back = INIFile::unescape(there);

		QCOMPARE(back, through);
	}

	void test_SaveLoad()
	{
		QString a = "a";
		QString b = "a\nb\t\n\\\\\\C:\\Program files\\terrible\\name\\of something\\";
		QString filename = "test_SaveLoad.ini";

		// save
		INIFile f;
		f.set("a", a);
		f.set("b", b);
		f.saveFile(filename);

		// load
		INIFile f2;
		f2.loadFile(filename);
		QCOMPARE(a, f2.get("a","NOT SET").toString());
		QCOMPARE(b, f2.get("b","NOT SET").toString());
	}
};

MMCTEST_GUILESS_MAIN(IniFileTest)

#include "tst_INIFile.moc"
