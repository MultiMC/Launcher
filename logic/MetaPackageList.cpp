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

#include <QtXml>
#include <QtAlgorithms>
#include <QtNetwork>

#include "Env.h"
#include "Exception.h"

#include "MetaPackageList.h"
#include "net/URLConstants.h"

#include "Json.h"
#include "ParseUtils.h"
#include "MetaPackage.h"
#include "FileStore.h"

#include <pathutils.h>

class ListLoadError : public Exception
{
public:
	ListLoadError(QString cause) : Exception(cause) {}
	virtual ~ListLoadError() noexcept
	{
	}
};

//FIXME: CachedListLoadTask and CachedVersionUpdateTask are almost identical...

class CachedListLoadTask : public Task
{
	Q_OBJECT

public:
	explicit CachedListLoadTask(MetaPackageList* vlist)
	{
		m_list = vlist;
	}
	virtual ~CachedListLoadTask() override{};

	virtual void executeTask() override
	{
		setStatus(tr("Loading version list..."));
		m_dlJob.reset(new NetJob("Version index"));
		auto entry = ENV.metacache()->resolveEntry("cache", QString("versions/%1.json").arg(m_list->uid()));
		entry->stale = true;

		m_dl = CacheDownload::make(QUrl(m_list->indexUrl()), entry);
		m_dlJob->addNetAction(m_dl);


		connect(m_dlJob.get(), SIGNAL(succeeded()), SLOT(list_downloaded()));
		connect(m_dlJob.get(), SIGNAL(failed(QString)), SIGNAL(failed(QString)));
		connect(m_dlJob.get(), SIGNAL(progress(qint64, qint64)),SIGNAL(progress(qint64, qint64)));
		m_dlJob->start();
	}

protected
slots:
	void list_downloaded()
	{
		QFile input(m_dl->getTargetFilepath());
		if(!input.open(QIODevice::ReadOnly))
		{
			emitFailed(tr("Error opening downloaded version index: %1").arg(m_dl->getTargetFilepath()));
			return;
		}
		QByteArray data = input.readAll();
		input.close();
		try
		{
			QJsonParseError jsonError;
			QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
			if (jsonError.error != QJsonParseError::NoError)
			{
				emitFailed(tr("Error parsing version list JSON: %1").arg(jsonError.errorString()));
				return;
			}
			m_list->loadListFromJSON(jsonDoc, MetaPackageList::RemoteLoaded);
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
	MetaPackageList *m_list;
	NetJobPtr m_dlJob;
	CacheDownloadPtr m_dl;
};

class CachedVersionUpdateTask : public Task
{
	Q_OBJECT

public:
	CachedVersionUpdateTask(MetaPackageList *vlist, QString updatedVersion)
		: Task()
	{
		m_list = vlist;
		versionToUpdate = updatedVersion;
	}
	virtual ~CachedVersionUpdateTask() override{};

	void executeTask()
	{
		m_dlJob.reset(new NetJob("Specific version download"));
		auto entry = ENV.metacache()->resolveEntry("cache", QString("versions/%1/%2.json").arg(m_list->uid(), versionToUpdate));
		entry->stale = true;
		m_dl = CacheDownload::make(QUrl(m_list->fileUrl(versionToUpdate)),entry);
		m_dlJob->addNetAction(m_dl);

		connect(m_dlJob.get(), SIGNAL(succeeded()), SLOT(json_downloaded()));
		connect(m_dlJob.get(), SIGNAL(failed(QString)), SIGNAL(failed(QString)));
		connect(m_dlJob.get(), SIGNAL(progress(qint64, qint64)), SIGNAL(progress(qint64, qint64)));
		m_dlJob->start();
	}

protected
slots:
	void json_downloaded()
	{
		QFile input(m_dl->getTargetFilepath());
		if(!input.open(QIODevice::ReadOnly))
		{
			emitFailed(tr("Error opening downloaded version: %1").arg(m_dl->getTargetFilepath()));
			return;
		}
		QByteArray data = input.readAll();
		input.close();

		QJsonParseError jsonError;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);

		if (jsonError.error != QJsonParseError::NoError)
		{
			emitFailed(tr("The download version file is not valid."));
			return;
		}
		emitSucceeded();
	}

protected:
	NetJobPtr m_dlJob;
	QString versionToUpdate;
	CacheDownloadPtr m_dl;
	MetaPackageList *m_list;
};

MetaPackageList::MetaPackageList(QString baseUrl, QString uid, QObject *parent) : BaseVersionList(parent)
{
	m_baseUrl = baseUrl;
	m_uid = uid;
	auto entry = ENV.metacache()->resolveEntry("cache", QString("versions/%1.json").arg(uid));
	QFile input(entry->getFullPath());
	if(!input.open(QIODevice::ReadOnly))
	{
		return;
	}
	QByteArray data = input.readAll();
	input.close();

	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
	loadListFromJSON(jsonDoc, MetaPackageList::LocalLoaded);
}

QString MetaPackageList::versionFilePath(QString version)
{
	if(m_lookup.contains(version))
	{
		auto entry = ENV.metacache()->resolveEntry("cache", QString("versions/%1/%2.json").arg(uid(), version));
		return entry->getFullPath();
	}
	return QString();
}

Task *MetaPackageList::getLoadTask()
{
	return new CachedListLoadTask(this);
}

bool MetaPackageList::isLoaded()
{
	return m_loaded != MetaPackageList::NotLoaded;
}

const BaseVersionPtr MetaPackageList::at(int i) const
{
	return m_vlist.at(i);
}

int MetaPackageList::count() const
{
	return m_vlist.count();
}

void MetaPackageList::sortInternal()
{
	auto cmpF = [](BaseVersionPtr first, BaseVersionPtr second)
	{
		auto left = std::dynamic_pointer_cast<MetaPackage>(first);
		auto right = std::dynamic_pointer_cast<MetaPackage>(second);
		return left->timestamp() > right->timestamp();
	};
	qSort(m_vlist.begin(), m_vlist.end(), cmpF);
}

void MetaPackageList::loadListFromJSON(QJsonDocument jsonDoc, LoadStatus source)
{
	qDebug() << "Loading version list.";

	if (!jsonDoc.isObject())
	{
		throw ListLoadError(tr("Error parsing version list JSON: jsonDoc is not an object"));
	}

	QJsonObject root = jsonDoc.object();

	// Now, get the array of versions.
	if (!root.value("versions").isArray())
	{
		throw ListLoadError(tr("Error parsing version list JSON: version list object is "
							   "missing 'versions' array"));
	}
	QJsonArray versions = root.value("versions").toArray();

	QList<BaseVersionPtr> tempList;
	for (auto version : versions)
	{
		// Load the version info.
		if (!version.isObject())
		{
			qCritical() << "Error while parsing version list : invalid JSON structure";
			continue;
		}

		QJsonObject versionObj = version.toObject();
		QString versionID = versionObj.value("id").toString("");
		if (versionID.isEmpty())
		{
			qCritical() << "Error while parsing version : version ID is missing";
			continue;
		}

		// Now, we construct the version object and add it to the list.
		auto rVersion = std::make_shared <MetaPackage>();
		rVersion->m_id = versionID;
		rVersion->m_time = versionObj.value("time").toDouble();
		rVersion->m_type = versionObj.value("type").toString("");
		tempList.append(rVersion);
	}
	if(m_loaded < source)
		m_loaded = source;
	updateListData(tempList);
}

void MetaPackageList::sort()
{
	beginResetModel();
	sortInternal();
	endResetModel();
}

BaseVersionPtr MetaPackageList::getLatestStable() const
{
	if(m_lookup.contains(m_latestReleaseID))
		return m_lookup[m_latestReleaseID];
	return BaseVersionPtr();
}

void MetaPackageList::reindex()
{
	m_lookup.clear();
	for(auto &version: m_vlist)
	{
		auto descr = version->descriptor();
		m_lookup[version->descriptor()] = version;
	}
}

void MetaPackageList::updateListData(QList<BaseVersionPtr> versions)
{
	beginResetModel();
	m_vlist = versions;
	reindex();
	sortInternal();
	endResetModel();
}

std::shared_ptr<Task> MetaPackageList::createUpdateTask(QString version)
{
	return std::shared_ptr<Task>(new CachedVersionUpdateTask(this, version));
}

#include "MetaPackageList.moc"
