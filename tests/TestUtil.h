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
};
class TestsInternal
{
public:
	static void setupTestingEnv()
	{
		QTemporaryDir dir;
		dir.setAutoRemove(false);
		ENV.m_icons.reset(new IconList(dir.path(), dir.path()));
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
		QTemporaryDir dir;
		dir.setAutoRemove(false);

		auto global = std::make_shared<DummySettings>();
		global->registerSetting("PreLaunchCommand");
		global->registerSetting("PostExitCommand");
		global->registerSetting("ShowConsole");
		global->registerSetting("AutoCloseConsole");
		global->registerSetting("LogPrePostOutput");
		return std::make_shared<NullInstance>(global,
											  std::make_shared<DummySettings>(),
											  dir.path());
	}
};

#define MULTIMC_GET_TEST_FILE(file) TestsInternal::readFile(QFINDTESTDATA(file))
#define MULTIMC_GET_TEST_FILE_UTF8(file) TestsInternal::readFileUtf8(QFINDTESTDATA(file))
