#pragma once

#include <QString>
#include <string>

/** Class for creating minimal Mac app bundles. */
class MacBundle
{
public:
	/** Create a MacBundle instance representing the bundle
	 * in <path>/<appName>.app
	 */
	MacBundle(const QString& path, const QString& appName);

	/** Create a simple Mac bundle.
	 *
	 * @param infoPlist The content of the Info.plist file
	 * @param icon The content of the app icon
	 * @param exePath The path of the file to use for the main app in the bundle.
	 */
	void create(const std::string& infoPlist,
				const std::string& icon,
				const QString& exePath);

	/** Returns the path of the main executable within the Mac bundle. */
	QString executablePath() const;

	/** Returns the path of the bundle */
	QString bundlePath() const;

private:
	QString m_path;
	QString m_appName;
	QString m_exePath;
};

