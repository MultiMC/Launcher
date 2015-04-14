#pragma once

#include <QFile>
#include <QCoreApplication>
#include <QTest>
#include <QDir>
#include <QTemporaryDir>

#include "test_config.h"
#include "NullInstance.h"
#include "settings/SettingsObject.h"
#include "Env.h"
#include "icons/IconList.h"

class DummySettings : public SettingsObject
{
protected:
	void changeSetting(const Setting &setting, QVariant value) override {}
	void resetSetting(const Setting &setting) override {}
	QVariant retrieveValue(const Setting &setting) override { return QVariant(); }
	void suspendSave() override {}
	void resumeSave() override {}
};
class TestsInternal
{
public:
	static void setupTestingEnv()
	{
		const QString dir = directory("icons");
		ENV.m_icons.reset(new IconList(dir, dir));
	}

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
	static InstancePtr createInstance()
	{
		const QString dir = directory("instance");

		auto global = std::make_shared<DummySettings>();
		global->registerSetting("PreLaunchCommand");
		global->registerSetting("PostExitCommand");
		global->registerSetting("ShowConsole");
		global->registerSetting("AutoCloseConsole");
		global->registerSetting("LogPrePostOutput");
		global->registerSetting("WrapperCommand");
		return std::make_shared<NullInstance>(global,
											  std::make_shared<DummySettings>(),
											  dir);
	}

	static bool compareFiles(const QString &f1, const QString &f2)
	{
		QFile file1(f1);
		QFile file2(f2);
		if (file1.size() != file2.size())
		{
			return false;
		}
		Q_ASSERT(file1.open(QFile::ReadOnly));
		Q_ASSERT(file2.open(QFile::ReadOnly));
		return file1.readAll() == file2.readAll();
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
public slots:
	void init()
	{
		m_dir = new QTemporaryDir;
		QDir::setCurrent(m_dir->path());
	}
	void cleanup()
	{
		delete m_dir;
	}

protected:
	QDir directory() const { return QDir(m_dir->path()); }
};

#define MULTIMC_GET_TEST_FILE(file) TestsInternal::readFile(QFINDTESTDATA(file))
#define MULTIMC_GET_TEST_FILE_UTF8(file) TestsInternal::readFileUtf8(QFINDTESTDATA(file))
