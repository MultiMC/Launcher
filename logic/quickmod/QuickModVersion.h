#pragma once

#include <QString>
#include <QMap>
#include <QCryptographicHash>
#include <QUrl>
#include <QStringList>

#include "logic/lists/BaseVersionList.h"
#include "logic/BaseVersion.h"
#include "logic/net/NetJob.h"
#include "logic/net/CacheDownload.h"
#include "logic/tasks/Task.h"
#include "logic/quickmod/QuickMod.h"

class BaseInstance;
typedef std::shared_ptr<BaseInstance> InstancePtr;

class QuickModVersion;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;
Q_DECLARE_METATYPE(QuickModVersionPtr)

class QuickModVersion : public BaseVersion
{
public:
	enum DownloadType
	{
		Direct,
		Parallel,
		Sequential
	};
	enum InstallType
	{
		ForgeMod,
		ForgeCoreMod,
		Extract,
		ConfigPack,
		Group
	};
	struct Library
	{
		QString name;
		QUrl url;
	};

	QuickModVersion(QuickModPtr mod = 0, bool valid = true) : mod(mod), valid(valid)
	{
	}
	QuickModVersion(QuickModPtr mod, const QString &name, const QUrl &url, const QStringList &mc,
					const QString &forge = QString(),
					const QMap<QString, QString> &deps = QMap<QString, QString>(),
					const QMap<QString, QString> &recs = QMap<QString, QString>(),
					const QString &md5 = QString(), const DownloadType downloadType = Parallel)
		: mod(mod), valid(true), name_(name), url(url), compatibleVersions(mc),
		  forgeVersionFilter(forge), dependencies(deps), recommendations(recs),
		  md5(md5), downloadType(downloadType)
	{
	}

	void parse(const QJsonObject &object);

	QString descriptor()
	{
		return name_;
	}
	QString name()
	{
		return name_;
	}
	QString typeString() const
	{
		return type;
	}

	QuickModPtr mod;
	bool valid;
	QString name_;
	QString type;
	QUrl url;
	QStringList compatibleVersions;
	QString forgeVersionFilter;
	QMap<QString, QString> dependencies;
	QMap<QString, QString> recommendations;
	QMap<QString, QString> suggestions;
	QMap<QString, QString> breaks;
	QMap<QString, QString> conflicts;
	QMap<QString, QString> provides;
	QString md5;
	DownloadType downloadType;
	InstallType installType;
	QList<Library> libraries;

	bool operator==(const QuickModVersion &other) const
	{
		return mod == other.mod && valid == other.valid && name_ == other.name_ &&
			   url == other.url && compatibleVersions == other.compatibleVersions &&
			   forgeVersionFilter == other.forgeVersionFilter &&
			   dependencies == other.dependencies && recommendations == other.recommendations &&
			   md5 == other.md5;
	}

	static QuickModVersionPtr invalid(QuickModPtr mod);
};

class QuickModVersionList : public BaseVersionList
{
	Q_OBJECT
public:
	explicit QuickModVersionList(QuickModPtr mod, InstancePtr instance, QObject *parent = 0);

	Task *getLoadTask();
	bool isLoaded();
	const BaseVersionPtr at(int i) const;
	int count() const;
	void sort() {}

protected
slots:
	void updateListData(QList<BaseVersionPtr> versions) {}

private:
	QuickModPtr m_mod;
	InstancePtr m_instance;
};
