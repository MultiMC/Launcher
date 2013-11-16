#pragma once

#include <QObject>
#include <QUrl>
#include <QFileInfo>
#include <QStringList>

class QFile;

class QuickModFile;

class QuickModVersion : public QObject
{
	Q_OBJECT

public:
	QuickModVersion(QuickModFile* parent = 0);

	enum ModType
	{
		ForgeMod,
		ForgeCoreMod,
		ResourcePack,
		ConfigPack
	};

	QuickModFile* qmFile() const { return m_qmFile; }
	QString name() const { return m_name; }
	QList<QUrl> urls() const { return m_urls; }
	QStringList mcVersions() const { return m_mcVersions; }
	ModType modType() const { return m_modType; }
	QString modTypeString() const;

private:
	friend class QuickModFile;
	friend class QuickModTreeModel;
	QuickModFile* m_qmFile;
	QString m_name;
	QList<QUrl> m_urls;
	QList<QString> m_mcVersions;
	ModType m_modType;
};

class QuickModFile : public QObject
{
	Q_OBJECT

public:
	QuickModFile(QObject* parent = 0);

	void setFileInfo(const QFileInfo& info);
	QFileInfo fileInfo() const { return m_info; }
	QFile* file() const { return m_file; }
	QString errorString() const { return m_errorString; }

	bool open();

	QString name() const { return m_name; }
	QUrl website() const { return m_website; }
	QUrl icon() const { return m_icon; }
	QList<QuickModVersion*> versions() const { return m_versions; }

private:
	friend class QuickModTreeModel;

	bool parse();

	QFileInfo m_info;
	QFile* m_file;
	QString m_errorString;

	QString m_name;
	QUrl m_website;
	QUrl m_icon;
	QList<QuickModVersion*> m_versions;
};
