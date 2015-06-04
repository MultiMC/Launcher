#include "MacBundle.h"

#include "FileUtils.h"
#include "Log.h"

MacBundle::MacBundle(const QString &path, const QString &appName)
	: m_appName(appName)
{
	m_path = path + '/' + appName + ".app";
}

QString MacBundle::bundlePath() const
{
	return m_path;
}

void MacBundle::create(const std::string& infoPlist,
					   const std::string& icon,
					   const QString& exePath)
{
	try
	{
		// create the bundle directories
		FileUtils::mkpath(m_path);

		const QString contentDir = m_path + "/Contents";
		const QString resourceDir = contentDir + "/Resources";
		const QString binDir = contentDir + "/MacOS";

		FileUtils::mkpath(resourceDir);
		FileUtils::mkpath(binDir);

		// create the Contents/Info.plist file
		FileUtils::writeFile(contentDir + "/Info.plist", QString::fromStdString(infoPlist).toUtf8());

		// save the icon to Contents/Resources/<appname>.icns
		FileUtils::writeFile(resourceDir + '/' + m_appName + ".icns", QString::fromStdString(icon).toUtf8());

		// copy the app binary to Contents/MacOS/<appname>
		m_exePath = binDir + '/' + m_appName;
		FileUtils::copyFile(exePath, m_exePath);
	}
	catch (const FileUtils::IOException& exception)
	{
		LOG(Error,"Unable to create app bundle. " + std::string(exception.what()));
	}
}

QString MacBundle::executablePath() const
{
	return m_exePath;
}

