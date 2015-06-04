#pragma once

#include <vector>
#include <QString>

class QXmlStreamReader;

/** Represents a file to be installed as part of an update. */
struct UpdateScriptFile
{
	explicit UpdateScriptFile()
	{
	}
	explicit UpdateScriptFile(const QString &src, const QString &dest, const int permissions)
		: source(src), dest(dest), permissions(permissions)
	{
	}

	/// Path to copy from.
	QString source;
	/// The path to copy to.
	QString dest;

	/** The permissions for this file, specified
		  * using the standard Unix mode_t values.
		  */
	int permissions = 0;

	bool operator==(const UpdateScriptFile &other) const
	{
		return source == other.source && dest == other.dest && permissions == other.permissions;
	}
};

/** Stores information about the files included in an update, parsed from an XML file. */
class UpdateScript
{
public:
	UpdateScript();

	/** Initialize this UpdateScript with the script stored
		  * in the XML file at @p path.
		  */
	void parse(const QString &path);

	bool isValid() const;
	const QString path() const;
	const std::vector<UpdateScriptFile> &filesToInstall() const;
	const std::vector<QString> &filesToUninstall() const;

private:
	void parseUpdate(QXmlStreamReader &reader);
	UpdateScriptFile parseFile(QXmlStreamReader &reader);

	QString m_path;
	std::vector<UpdateScriptFile> m_filesToInstall;
	std::vector<QString> m_filesToUninstall;
};
