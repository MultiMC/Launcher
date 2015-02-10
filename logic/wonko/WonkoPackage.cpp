/* Copyright 2013-2014 MultiMC Contributors
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

#include "WonkoPackage.h"

#include "Env.h"
#include "Exception.h"
#include "Json.h"

#include "net/URLConstants.h"

#include "WonkoPackageVersion.h"
#include "FileStore.h"

#include <pathutils.h>

class ListLoadError : public Exception
{
public:
	ListLoadError(QString cause) : Exception(cause)
	{
	}
};

// FIXME: CachedListLoadTask and CachedVersionUpdateTask are almost identical...

class CachedListLoadTask : public Task
{
	Q_OBJECT
public:
	explicit CachedListLoadTask(WonkoPackage *vlist) : Task(), m_list(vlist)
	{
	}

	void executeTask() override
	{
		setStatus(tr("Loading version list..."));
		m_dlJob = new NetJob("Version index");
		auto entry = ENV.metacache()->resolveEntry(
			"cache", QString("versions/%1.json").arg(m_list->uid()));
		entry->stale = true;

		m_dl = CacheDownload::make(QUrl(m_list->indexUrl()), entry);
		m_dl->setValidator(new JsonValidator);
		m_dlJob->addNetAction(m_dl);

		connect(m_dlJob.get(), &NetJob::succeeded, this, &CachedListLoadTask::list_downloaded);
		connect(m_dlJob.get(), &NetJob::failed, [this](QString reason)
				{
			emitFailed(m_dlJob->failReason());
		});
		connect(m_dlJob.get(), &NetJob::progress, this, &CachedListLoadTask::progress);
		m_dlJob->start();
	}

protected slots:
	void list_downloaded()
	{
		try
		{
			const QJsonDocument doc = Json::ensureDocument(m_dl->getTargetFilepath());
			m_list->loadListFromJSON(doc, WonkoPackage::RemoteLoaded);
		}
		catch (Exception &e)
		{
			emitFailed(e.cause());
			return;
		}

		emitSucceeded();
		return;
	}

protected:
	WonkoPackage *m_list;
	NetJobPtr m_dlJob;
	CacheDownloadPtr m_dl;
};

class CachedVersionUpdateTask : public Task
{
	Q_OBJECT

public:
	CachedVersionUpdateTask(WonkoPackage *vlist, const QString &updatedVersion)
		: Task(), m_versionToUpdate(updatedVersion), m_list(vlist)
	{
	}

	void executeTask()
	{
		m_dlJob = new NetJob("Specific version download");
		auto entry = ENV.metacache()->resolveEntry(
			"cache", QString("versions/%1/%2.json").arg(m_list->uid(), m_versionToUpdate));
		entry->stale = true;
		CacheDownloadPtr dl =
			CacheDownload::make(QUrl(m_list->fileUrl(m_versionToUpdate)), entry);
		dl->setValidator(new JsonValidator);
		m_dlJob->addNetAction(dl);

		connect(m_dlJob.get(), &NetJob::succeeded, this,
				&CachedVersionUpdateTask::emitSucceeded);
		connect(m_dlJob.get(), &NetJob::failed, [this](QString reason)
				{
			emitFailed(m_dlJob->failReason());
		});
		connect(m_dlJob.get(), &NetJob::progress, this, &CachedVersionUpdateTask::progress);
		connect(m_dlJob.get(), &NetJob::status, this, &CachedVersionUpdateTask::setStatus);
		m_dlJob->start();
	}

protected:
	NetJobPtr m_dlJob;
	QString m_versionToUpdate;
	WonkoPackage *m_list;
};

WonkoPackage::WonkoPackage(const QString &baseUrl, const QString &uid, QObject *parent)
	: BaseVersionList(parent), m_baseUrl(baseUrl), m_uid(uid)
{
	// stupid hack required because we use shared_from_this() in loadFromIndex, which is called
	// by loadListFromJSON
	QMetaObject::invokeMethod(this, "initialLoad", Qt::QueuedConnection);
}

void WonkoPackage::loadFromIndex(const QJsonObject &obj,
									 const WonkoPackage::LoadStatus source)
{
	if (m_isFull)
	{
		return;
	}

	const QString uid = Json::ensureString(obj, "uid");
	if (uid != m_uid)
	{
		throw ListLoadError("Invalid UID (expected " + m_uid + ", got " + uid + ")");
	}

	if (m_loaded < source)
	{
		m_loaded = source;
	}
}

QString WonkoPackage::versionFilePath(QString version) const
{
	if (m_lookup.contains(version))
	{
		auto entry = ENV.metacache()->resolveEntry(
			"cache", QString("versions/%1/%2.json").arg(uid(), version));
		return entry->getFullPath();
	}
	return QString();
}

void WonkoPackage::initialLoad()
{
	auto entry = ENV.metacache()->resolveEntry("cache", QString("versions/%1.json").arg(m_uid));
	if (!QFile::exists(entry->getFullPath()))
	{
		return;
	}
	try
	{
		loadListFromJSON(Json::ensureDocument(entry->getFullPath()),
						 WonkoPackage::LocalLoaded);
	}
	catch (Exception &e)
	{
		qWarning() << "Unable to read version list for" << m_uid << ":" << e.cause();
	}
}

Task *WonkoPackage::getLoadTask()
{
	return new CachedListLoadTask(this);
}
bool WonkoPackage::isLoaded()
{
	return m_loaded != WonkoPackage::NotLoaded && m_isFull;
}
const BaseVersionPtr WonkoPackage::at(int i) const
{
	return m_vlist.at(i);
}
int WonkoPackage::count() const
{
	return m_vlist.count();
}

void WonkoPackage::loadListFromJSON(const QJsonDocument &jsonDoc, const LoadStatus source)
{
	qDebug() << "Loading version list.";

	try
	{
		using namespace Json;
		const QJsonObject root = ensureObject(jsonDoc);
		const QString uid = ensureString(root, "uid");

		QList<BaseVersionPtr> tempList;
		for (const QJsonObject &versionObj : ensureIsArrayOf<QJsonObject>(root, "versions"))
		{
			auto rVersion = std::make_shared<WonkoPackageVersion>();
			rVersion->load(versionObj, uid);
			tempList.append(rVersion);
		}

		// loadFromIndex handles loading of some basic metadata and some housekeeping like
		// updating m_loaded. We do this after reading after parsing the versions to catch any
		// errors occuring during the loading before setting any state
		loadFromIndex(root, source);

		updateListData(tempList);

		m_isFull = true;
	}
	catch (Exception &e)
	{
		throw ListLoadError("Error while parsing version list JSON: " + e.cause());
	}
}

void WonkoPackage::sortInternal()
{
	auto cmpF = [](BaseVersionPtr first, BaseVersionPtr second)
	{
		auto left = std::dynamic_pointer_cast<WonkoPackageVersion>(first);
		auto right = std::dynamic_pointer_cast<WonkoPackageVersion>(second);
		return left->timestamp() > right->timestamp();
	};
	std::sort(m_vlist.begin(), m_vlist.end(), cmpF);
}
void WonkoPackage::sort()
{
	beginResetModel();
	sortInternal();
	endResetModel();
}

BaseVersionPtr WonkoPackage::getLatestStable() const
{
	if (m_lookup.contains(m_latestReleaseID))
	{
		return m_lookup[m_latestReleaseID];
	}
	return BaseVersionPtr();
}

void WonkoPackage::reindex()
{
	m_lookup.clear();
	for (auto &version : m_vlist)
	{
		m_lookup[version->descriptor()] = version;
	}
}

void WonkoPackage::updateListData(QList<BaseVersionPtr> versions)
{
	beginResetModel();
	m_vlist = versions;
	reindex();
	sortInternal();
	endResetModel();
}

std::shared_ptr<Task> WonkoPackage::createUpdateTask(const QString &version)
{
	return std::make_shared<CachedVersionUpdateTask>(this, version);
}

QVariant WonkoPackage::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() > count())
		return QVariant();

	auto version = std::dynamic_pointer_cast<WonkoPackageVersion>(m_vlist[index.row()]);
	switch (role)
	{
	case VersionPointerRole:
		return qVariantFromValue(m_vlist[index.row()]);


	case VersionRole:
		return version->name();

	case VersionIdRole:
		return version->descriptor();

	case RecommendedRole:
		return false;

	case TypeRole:
		return version->typeString();

	default:
		return QVariant();
	}
}

BaseVersionList::RoleList WonkoPackage::providesRoles()
{
	return {VersionPointerRole, VersionRole, VersionIdRole, RecommendedRole, TypeRole};
}

#include "WonkoPackage.moc"
