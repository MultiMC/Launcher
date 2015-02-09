#include "UpdateScript.h"

#include <QXmlStreamReader>
#include <QFile>
#include <stdexcept>

UpdateScript::UpdateScript()
{
}

void UpdateScript::parse(const QString &path)
{
	m_path.clear();

	QFile file(path);
	if (!file.open(QFile::ReadOnly))
	{
		throw std::runtime_error("Unable to open script file: " +
								 file.errorString().toStdString());
	}
	QXmlStreamReader reader(&file);
	if (!reader.readNextStartElement())
	{
		throw std::runtime_error("Invalid XML document");
	}
	if (reader.name() != "update")
	{
		throw std::runtime_error("Not an updater script");
	}
	parseUpdate(reader);
	if (reader.hasError())
	{
		throw std::runtime_error("Unable to read script: " +
								 reader.errorString().toStdString());
	}

	m_path = path;
}

bool UpdateScript::isValid() const
{
	return !m_path.isEmpty();
}

void UpdateScript::parseUpdate(QXmlStreamReader &reader)
{
	bool inInstall = false;
	bool inUninstall = false;
	while (!reader.atEnd() && !reader.hasError())
	{
		const QXmlStreamReader::TokenType type = reader.readNext();
		if (type == QXmlStreamReader::StartElement)
		{
			if (reader.name() == "install")
			{
				inInstall = true;
			}
			else if (reader.name() == "uninstall")
			{
				inUninstall = true;
			}
			else if (reader.name() == "file")
			{
				if (inInstall)
				{
					m_filesToInstall.push_back(parseFile(reader));
				}
				else if (inUninstall)
				{
					m_filesToUninstall.push_back(reader.readElementText());
				}
			}
			else
			{
				throw std::runtime_error("Unknown XML tag in script: " +
										 reader.name().toString().toStdString());
			}
		}
		else if (type == QXmlStreamReader::EndElement)
		{
			if (reader.name() == "install")
			{
				inInstall = false;
			}
			else if (reader.name() == "uninstall")
			{
				inUninstall = false;
			}
		}
	}
}

UpdateScriptFile UpdateScript::parseFile(QXmlStreamReader &reader)
{
	UpdateScriptFile file;
	while (!reader.hasError() && !reader.atEnd())
	{
		const QXmlStreamReader::TokenType type = reader.readNext();
		if (type == QXmlStreamReader::StartElement)
		{
			if (reader.name() == "source")
			{
				file.source = reader.readElementText();
			}
			else if (reader.name() == "dest")
			{
				file.dest = reader.readElementText();
			}
			else if (reader.name() == "mode")
			{
				file.permissions = reader.readElementText().toInt(nullptr, 8);
			}
			else
			{
				throw std::runtime_error("Unknown XML tag in script: " +
										 reader.name().toString().toStdString());
			}
		}
		else if (type == QXmlStreamReader::EndElement)
		{
			if (reader.name() == "file")
			{
				break;
			}
		}
	}

	return file;
}

const std::vector<UpdateScriptFile> &UpdateScript::filesToInstall() const
{
	return m_filesToInstall;
}

const std::vector<QString> &UpdateScript::filesToUninstall() const
{
	return m_filesToUninstall;
}

const QString UpdateScript::path() const
{
	return m_path;
}
