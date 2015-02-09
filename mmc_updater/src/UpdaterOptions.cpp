#include "UpdaterOptions.h"

#include <QCommandLineParser>

void UpdaterOptions::parse(int argc, char **argv)
{
	QStringList args;
	for (int i = 0; i < argc; ++i)
	{
		args += QString::fromLocal8Bit(argv[i]);
	}

	QCommandLineParser parser;
	parser.addOption(QCommandLineOption("install-dir", "", "DIR"));
	parser.addOption(QCommandLineOption("package-dir", "", "DIR"));
	parser.addOption(QCommandLineOption("script", "", "FILE"));
	parser.addOption(QCommandLineOption("wait", "", "PID"));
	parser.addOption(QCommandLineOption("dry-run"));
	parser.addOption(QCommandLineOption("finish-cmd", "", "CMD"));
	parser.addOption(QCommandLineOption("finish-dir", "", "DIR"));
	parser.addVersionOption();
	parser.process(args);

	if (parser.isSet("install-dir"))
	{
		installDir = parser.value("install-dir");
	}
	if (parser.isSet("package-dir"))
	{
		packageDir = parser.value("package-dir");
	}
	if (parser.isSet("script"))
	{
		scriptPath = parser.value("script");
	}
	if (parser.isSet("wait"))
	{
		waitPid = parser.value("wait").toULongLong();
	}
	if (parser.isSet("finish-cmd"))
	{
		finishCmd = parser.value("finish-cmd");
	}
	if (parser.isSet("finish-dir"))
	{
		finishDir = parser.value("finish-dir");
	}

	dryRun = parser.isSet("dry-run");
}
