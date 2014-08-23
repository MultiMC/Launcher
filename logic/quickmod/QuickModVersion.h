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

#include "logic/BaseVersion.h"
#include "logic/net/NetJob.h"
#include "logic/net/CacheDownload.h"
#include "logic/tasks/Task.h"
#include "logic/quickmod/QuickMod.h"
#include "QuickModDownload.h"
#include "QuickModVersionRef.h"
#include "modutils.h"

class BaseInstance;
typedef std::shared_ptr<BaseInstance> InstancePtr;

class QuickModVersion;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;
Q_DECLARE_METATYPE(QuickModVersionPtr)

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
		Library()
		{
		}
		Library(const QString &name, const QUrl &url) : name(name), url(url)
		{
		}
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

	QuickModVersionRef version() const
	{
		return QuickModVersionRef(mod->uid(), version_string.isNull() ? name_ : version_string, m_version);
	}

	bool needsDeploy() const;

	QuickModPtr mod;
	bool valid;
	QString name_;
	QString version_string;
	QString type;
	QStringList compatibleVersions;
	QString forgeVersionFilter;
	QString liteloaderVersionFilter;
	QMap<QuickModRef, QPair<QuickModVersionRef, bool>> dependencies;
	QMap<QuickModRef, QuickModVersionRef> recommendations;
	QMap<QuickModRef, QuickModVersionRef> suggestions;
	QMap<QuickModRef, QuickModVersionRef> conflicts;
	QMap<QuickModRef, QuickModVersionRef> provides;
	QString sha1;
	InstallType installType;
	QList<Library> libraries;
	QList<QuickModDownload> downloads;
	Util::Version m_version;

	bool operator==(const QuickModVersion &other) const
	{
		return mod == other.mod && valid == other.valid && name_ == other.name_ &&
			   version_string == other.version_string && compatibleVersions == other.compatibleVersions &&
			   forgeVersionFilter == other.forgeVersionFilter &&
			   dependencies == other.dependencies && recommendations == other.recommendations &&
			   sha1 == other.sha1;
	}
};
