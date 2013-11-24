#pragma once

#include <QFile>
#include <QCoreApplication>

#include "logic/lists/QuickModsList.h"
#include "MultiMC.h"

typedef QList<QuickMod::Version> QMVersionList;
struct TestsInternal
{
	static QByteArray readFile(const QString &fileName)
	{
		QFile f(fileName);
		f.open(QFile::ReadOnly);
		return f.readAll();
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

#define QTEST_GUILESS_MAIN_MULTIMC(TestObject) \
int main(int argc, char *argv[]) \
{ \
	char *argv_[1] = { argv[0] }; \
	int argc_ = 1; \
	MultiMC app(argc_, argv_, QDir::temp().absoluteFilePath("MultiMC_Test")); \
	app.setAttribute(Qt::AA_Use96Dpi, true); \
	TestObject tc; \
	return QTest::qExec(&tc, argc, argv); \
}
