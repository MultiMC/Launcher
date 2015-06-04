#include "AppInfo.h"

#include <QDir>

#include <string>

std::string AppInfo::logFilePath()
{
	return QDir::current().absoluteFilePath("update-log.txt").toStdString();
}

std::string AppInfo::updateErrorMessage(const std::string &details)
{
	std::string result = "There was a problem installing the update:\n\n";
	result += details;
	result += "\n\nYou can try downloading and installing the latest version of "
			  "MultiMC from http://multimc.org/";
	return result;
}
