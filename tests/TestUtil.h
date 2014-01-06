#pragma once

#include <QFile>
#include <QCoreApplication>
#include <QTest>
#include <QDir>

#include "MultiMC.h"
#include "logic/lists/QuickModsList.h"

typedef QList<QuickMod::Version> QMVersionList;
struct TestsInternal
{
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

		static QuickMod *createMod(QString name, QMVersionList versions)
		{
			QuickMod *mod = new QuickMod;
			mod->m_name = name;
			mod->m_nemName = name;
			mod->m_modId = name;
			mod->m_description = name + " description";
			mod->m_updateUrl = "http://localhost/quickmod/" + name + ".json";
			mod->m_versions = versions;
			return mod;
		}
		static QuickMod *createMod(QString name)
		{
			return createMod(name, QMVersionList() << QuickMod::Version("1.0.0", QUrl("http://localhost/" + name + ".jar"), QStringList() << "1.6.2" << "1.6.4"));
		}
};

#define MULTIMC_GET_TEST_FILE(file) TestsInternal::readFile(QFINDTESTDATA( file ))
#define MULTIMC_GET_TEST_FILE_UTF8(file) TestsInternal::readFileUtf8(QFINDTESTDATA( file ))

#ifdef Q_OS_LINUX
# define _MMC_EXTRA_ARGV , "-platform", "offscreen"
# define _MMC_EXTRA_ARGC 2
#else
# define _MMC_EXTRA_ARGV
# define _MMC_EXTRA_ARGC 0
#endif
	
	
	
#define QTEST_GUILESS_MAIN_MULTIMC(TestObject) \
int main(int argc, char *argv[]) \
{ \
	char *argv_[] = { argv[0] _MMC_EXTRA_ARGV }; \
	int argc_ = 1 + _MMC_EXTRA_ARGC; \
	MultiMC app(argc_, argv_, true); \
	app.setAttribute(Qt::AA_Use96Dpi, true); \
	TestObject tc; \
	return QTest::qExec(&tc, argc, argv); \
}
