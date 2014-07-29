/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QString>
#include <QMap>
#include <QCryptographicHash>
#include <QUrl>
#include <QStringList>

#include "logic/BaseVersionList.h"
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

class QuickModDownload
{
public:
	enum DownloadType
	{
		Direct = 1,
		Parallel,
		Sequential,
		Encoded,
		Maven
	};

	QString url;
	DownloadType type;
	int priority;
	QString hint;
	QString group;

	QJsonObject toJson() const;
};

class QuickModVersion : public BaseVersion
{
public:
	enum InstallType
	{
		ForgeMod,
		ForgeCoreMod,
		LiteLoaderMod,
		Extract,
		ConfigPack,
		Group
	};
	struct Library
	{
		Library() {}
		Library(const QString &name, const QUrl &url)
			: name(name), url(url) {}
		QJsonObject toJson() const;
		QString name;
		QUrl url;
	};

	QuickModVersion(QuickModPtr mod = 0, bool valid = true) : mod(mod), valid(valid)
	{
	}

	void parse(const QJsonObject &object);
	QJsonObject toJson() const;

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

	bool needsDeploy() const;

	QuickModPtr mod;
	bool valid;
	QString name_;
	QString type;
	QStringList compatibleVersions;
	QString forgeVersionFilter;
	QString liteloaderVersionFilter;
	QMap<QuickModUid, QPair<QString, bool>> dependencies;
	QMap<QuickModUid, QString> recommendations;
	QMap<QuickModUid, QString> suggestions;
	QMap<QuickModUid, QString> conflicts;
	QMap<QuickModUid, QString> provides;
	QString sha1;
	InstallType installType;
	QList<Library> libraries;
	QList<QuickModDownload> downloads;

	bool operator==(const QuickModVersion &other) const
	{
		return mod == other.mod && valid == other.valid && name_ == other.name_ &&
			   compatibleVersions == other.compatibleVersions &&
			   forgeVersionFilter == other.forgeVersionFilter &&
			   dependencies == other.dependencies && recommendations == other.recommendations &&
			   sha1 == other.sha1;
	}

	static QuickModVersionPtr invalid(QuickModPtr mod);
};

class QuickModVersionList : public BaseVersionList
{
	Q_OBJECT
public:
	explicit QuickModVersionList(QuickModUid mod, InstancePtr instance, QObject *parent = 0);

	Task *getLoadTask();
	bool isLoaded();
	const BaseVersionPtr at(int i) const;
	int count() const;
	void sort()
	{
	}

protected
slots:
	void updateListData(QList<BaseVersionPtr> versions)
	{
	}

private:
	QuickModUid m_mod;
	InstancePtr m_instance;

	QList<QuickModVersionPtr> versions() const;
};
