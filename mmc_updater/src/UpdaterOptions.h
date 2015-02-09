#pragma once

#include <QString>

/** Parses the command-line options to the updater binary. */
class UpdaterOptions
{
public:
	void parse(int argc, char **argv);

	QString installDir;
	QString packageDir;
	QString scriptPath;
	QString finishCmd;
	QString finishDir;
	unsigned long long waitPid = 0;
	QString logFile;
	bool dryRun;
};
