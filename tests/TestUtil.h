#pragma once

#include <QFile>
#include <QCoreApplication>
#include <QTest>
#include <QDir>
#include <QTemporaryDir>
#include <QDebug>

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

	static bool compareFiles(const QString &f1, const QString &f2)
	{
		QFile file1(f1);
		QFile file2(f2);
		if (file1.size() != file2.size())
		{
			qDebug() << "Size of" << f1 << "and" << f2 << "differ, they are" << file1.size() << "and" << file2.size();
			return false;
		}
		Q_ASSERT(file1.open(QFile::ReadOnly));
		Q_ASSERT(file2.open(QFile::ReadOnly));
		const QByteArray data1 = file1.readAll();
		const QByteArray data2 = file2.readAll();
		if (data1 != data2)
		{
			qDebug() << "The contents of" << f1 << "and" << f2 << "differ";
			qDebug() << data1;
			qDebug() << data2;
			return false;
		}
		return true;
	}

	static int nextInteger()
	{
		static int integer = 0;
		return integer++;
	}
	static QString directory(const QString &base)
	{
		return QDir::current().absoluteFilePath(QString("%1-%2").arg(base).arg(nextInteger()));
	}
};

class BaseTest : public QObject
{
	Q_OBJECT

	QTemporaryDir *m_dir;
public
slots:
	void init()
	{
#ifdef SQUISHCOCO_COVERAGE
		__coveragescanner_clear();
#endif
		m_dir = new QTemporaryDir;
		QDir::setCurrent(m_dir->path());
	}
	void cleanup()
	{
#ifdef SQUISHCOCO_COVERAGE
		const QString testname = QString("unittests/%1/%2").arg(metaObject()->className(), QTest::currentTestFunction());
		qDebug() << "Will save" << testname;
		__coveragescanner_testname(testname.toLatin1());
		__coveragescanner_teststate(QTest::currentTestFailed() ? "FAILED" : "PASSED");
		__coveragescanner_save();
		__coveragescanner_testname("");
#endif
		delete m_dir;
	}

protected:
	QDir directory() const { return QDir(m_dir->path()); }
};

#define MULTIMC_GET_TEST_FILE(file) TestsInternal::readFile(QFINDTESTDATA(file))
#define MULTIMC_GET_TEST_FILE_UTF8(file) TestsInternal::readFileUtf8(QFINDTESTDATA(file))

#ifdef SQUISHCOCO_COVERAGE
static const char *currentExecutable = SQUISHCOCO_CURRENTEXECUTABLE;
# warning "Compiling with Squish Coco code coverage"
# define MMCTEST_GUILESS_MAIN(TestObject) \
	int main(int argc, char *argv[]) \
	{ \
		__coveragescanner_install(currentExecutable); \
		QCoreApplication app(argc, argv); \
		qDebug() << "Coverage reporting for" << currentExecutable << "enabled"; \
		app.setAttribute(Qt::AA_Use96Dpi, true); \
		TestObject tc; \
		return QTest::qExec(&tc, argc, argv); \
	}
#else
# define MMCTEST_GUILESS_MAIN(TestObject) \
	int main(int argc, char *argv[]) \
	{ \
		QCoreApplication app(argc, argv); \
		app.setAttribute(Qt::AA_Use96Dpi, true); \
		TestObject tc; \
		return QTest::qExec(&tc, argc, argv); \
	}
#endif
