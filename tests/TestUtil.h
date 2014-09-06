#pragma once

#include <QFile>
#include <QCoreApplication>
#include <QTest>
#include <QDir>

#include "MultiMC.h"
#include "logic/quickmod/QuickMod.h"

#include "test_config.h"

class TestsInternal
{
public:
	static QByteArray readFile(const QString &fileName)
	{
		QFile f(fileName);
		f.open(QFile::ReadOnly);
		return f.readAll();
	}
	static QString readFileUtf8(const QString &fileName)
	{
		return QString::fromUtf8(readFile(fileName));
	}

	static QuickModPtr createMod(QString name)
	{
		auto mod = QuickModPtr(new QuickMod);
		mod->m_uid = QuickModRef(name);
		mod->m_repo = "test_repo";
		mod->m_name = name;
		mod->m_modId = name;
		mod->m_description = name + " description";
		mod->m_updateUrl = "http://localhost/quickmod/" + name + ".json";
		return mod;
	}
};

#define MULTIMC_GET_TEST_FILE(file) TestsInternal::readFile(QFINDTESTDATA(file))
#define MULTIMC_GET_TEST_FILE_UTF8(file) TestsInternal::readFileUtf8(QFINDTESTDATA(file))

#ifdef Q_OS_LINUX
#define _MMC_EXTRA_ARGV , "-platform", "offscreen"
#define _MMC_EXTRA_ARGC 2
#else
#define _MMC_EXTRA_ARGV
#define _MMC_EXTRA_ARGC 0
#endif

#define QTEST_GUILESS_MAIN_MULTIMC(TestObject)                                                 \
	int main(int argc, char *argv[])                                                           \
	{                                                                                          \
		const char *argv_[] = {argv[0] _MMC_EXTRA_ARGV};                                       \
		int argc_ = 1 + _MMC_EXTRA_ARGC;                                                       \
		MultiMC app(argc_, const_cast<char **>(argv_), true);                                  \
		app.setAttribute(Qt::AA_Use96Dpi, true);                                               \
		TestObject tc;                                                                         \
		return QTest::qExec(&tc, argc, argv);                                                  \
	}
