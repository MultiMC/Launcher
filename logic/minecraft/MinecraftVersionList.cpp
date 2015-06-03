/* Copyright 2013-2015 MultiMC Contributors
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
#include "Json.h"
#include <QtAlgorithms>
#include <QtNetwork>

#include "Env.h"
#include "Exception.h"

#include "MinecraftVersionList.h"
#include "net/URLConstants.h"

#include "ParseUtils.h"
#include "ProfileUtils.h"
#include "VersionFilterData.h"

#include <pathutils.h>

static const char * localVersionCache = "versions/versions.dat";

class MCVListLoadTask : public Task
{
	Q_OBJECT

public:
	explicit MCVListLoadTask(MinecraftVersionList *vlist);
	virtual ~MCVListLoadTask() override{};

	virtual void executeTask() override;

protected
slots:
	void list_downloaded();

protected:
	QNetworkReply *vlistReply;
	MinecraftVersionList *m_list;
	MinecraftVersion *m_currentStable;
};

class MCVListVersionUpdateTask : public Task
{
	Q_OBJECT

public:
	explicit MCVListVersionUpdateTask(MinecraftVersionList *vlist, QString updatedVersion);
	virtual ~MCVListVersionUpdateTask() override{};
	virtual void executeTask() override;

protected
slots:
	void json_downloaded();

protected:
	NetJobPtr specificVersionDownloadJob;
	QString versionToUpdate;
	MinecraftVersionList *m_list;
};

class ListLoadError : public Exception
{
public:
	ListLoadError(QString cause) : Exception(cause) {};
	virtual ~ListLoadError() noexcept
	{
	}
};

MinecraftVersionList::MinecraftVersionList(QObject *parent) : BaseVersionList(parent)
{
	loadBuiltinList();
	loadCachedList();
}

Task *MinecraftVersionList::getLoadTask()
{
	return new MCVListLoadTask(this);
}

bool MinecraftVersionList::isLoaded()
{
	return m_loaded;
}

const BaseVersionPtr MinecraftVersionList::at(int i) const
{
	return m_vlist.at(i);
}

int MinecraftVersionList::count() const
{
	return m_vlist.count();
}

static bool cmpVersions(BaseVersionPtr first, BaseVersionPtr second)
{
	auto left = std::dynamic_pointer_cast<MinecraftVersion>(first);
	auto right = std::dynamic_pointer_cast<MinecraftVersion>(second);
	return left->m_releaseTime > right->m_releaseTime;
}

void MinecraftVersionList::sortInternal()
{
	qSort(m_vlist.begin(), m_vlist.end(), cmpVersions);
}

void MinecraftVersionList::loadCachedList()
{
	QFile localIndex(localVersionCache);
	if (!localIndex.exists())
	{
		return;
	}
	if (!localIndex.open(QIODevice::ReadOnly))
	{
		// FIXME: this is actually a very bad thing! How do we deal with this?
		qCritical() << "The minecraft version cache can't be read.";
		return;
	}
	auto data = localIndex.readAll();
	try
	{
		localIndex.close();
		QJsonDocument jsonDoc = QJsonDocument::fromBinaryData(data);
		if (jsonDoc.isNull())
		{
			throw ListLoadError(tr("Error reading the version list."));
		}
		loadMojangList(jsonDoc, Local);
	}
	catch (Exception &e)
	{
		// the cache has gone bad for some reason... flush it.
		qCritical() << "The minecraft version cache is corrupted. Flushing cache.";
		localIndex.remove();
		return;
	}
	m_hasLocalIndex = true;
}

void MinecraftVersionList::loadBuiltinList()
{
	qDebug() << "Loading builtin version list.";
	// grab the version list data from internal resources.
	const QJsonDocument doc =
		Json::ensureDocument(QString(":/versions/minecraft.json"), "builtin version list");
	const QJsonObject root = doc.object();

	// parse all the versions
	for (const auto version : Json::ensureArray(root.value("versions")))
	{
		QJsonObject versionObj = version.toObject();
		QString versionID = versionObj.value("id").toString("");
		QString versionTypeStr = versionObj.value("type").toString("");
		if (versionID.isEmpty() || versionTypeStr.isEmpty())
		{
			qCritical() << "Parsed version is missing ID or type";
			continue;
		}

		if (g_VersionFilterData.legacyBlacklist.contains(versionID))
		{
			qWarning() << "Blacklisted legacy version ignored: " << versionID;
			continue;
		}

		// Now, we construct the version object and add it to the list.
		std::shared_ptr<MinecraftVersion> mcVersion(new MinecraftVersion());
		mcVersion->m_name = mcVersion->m_descriptor = versionID;

		// Parse the timestamp.
		if (!parse_timestamp(versionObj.value("releaseTime").toString(""),
							 mcVersion->m_releaseTimeString, mcVersion->m_releaseTime))
		{
			qCritical() << "Error while parsing version" << versionID
						 << ": invalid version timestamp";
			continue;
		}

		// Get the download URL.
		mcVersion->download_url =
			"http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + versionID + "/";

		mcVersion->m_versionSource = Builtin;
		mcVersion->m_type = versionTypeStr;
		mcVersion->m_appletClass = versionObj.value("appletClass").toString("");
		mcVersion->m_mainClass = versionObj.value("mainClass").toString("");
		mcVersion->m_jarChecksum = versionObj.value("checksum").toString("");
		mcVersion->m_processArguments = versionObj.value("processArguments").toString("legacy");
		if (versionObj.contains("+traits"))
		{
			for (auto traitVal : Json::ensureArray(versionObj.value("+traits")))
			{
				mcVersion->m_traits.insert(Json::ensureString(traitVal));
			}
		}
		m_lookup[versionID] = mcVersion;
		m_vlist.append(mcVersion);
	}
}

void MinecraftVersionList::loadMojangList(QJsonDocument jsonDoc, VersionSource source)
{
	qDebug() << "Loading" << ((source == Remote) ? "remote" : "local") << "version list.";

	if (!jsonDoc.isObject())
	{
		throw ListLoadError(tr("Error parsing version list JSON: jsonDoc is not an object"));
	}

	QJsonObject root = jsonDoc.object();

	try
	{
		QJsonObject latest = Json::ensureObject(root.value("latest"));
		m_latestReleaseID = Json::ensureString(latest.value("release"));
		m_latestSnapshotID = Json::ensureString(latest.value("snapshot"));
	}
	catch (Exception &err)
	{
		qCritical()
			<< tr("Error parsing version list JSON: couldn't determine latest versions");
	}

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

		if (g_VersionFilterData.legacyBlacklist.contains(versionID))
		{
			qWarning() << "Blacklisted legacy version ignored: " << versionID;
			continue;
		}

		// Now, we construct the version object and add it to the list.
		std::shared_ptr<MinecraftVersion> mcVersion(new MinecraftVersion());
		mcVersion->m_name = mcVersion->m_descriptor = versionID;

		if (!parse_timestamp(versionObj.value("releaseTime").toString(""),
							 mcVersion->m_releaseTimeString, mcVersion->m_releaseTime))
		{
			qCritical() << "Error while parsing version" << versionID
						 << ": invalid release timestamp";
			continue;
		}
		if (!parse_timestamp(versionObj.value("time").toString(""),
							 mcVersion->m_updateTimeString, mcVersion->m_updateTime))
		{
			qCritical() << "Error while parsing version" << versionID
						 << ": invalid update timestamp";
			continue;
		}

		if (mcVersion->m_releaseTime < g_VersionFilterData.legacyCutoffDate)
		{
			continue;
		}

		// depends on where we load the version from -- network request or local file?
		mcVersion->m_versionSource = source;

		QString dlUrl = "http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + versionID + "/";
		mcVersion->download_url = dlUrl;
		QString versionTypeStr = versionObj.value("type").toString("");
		if (versionTypeStr.isEmpty())
		{
			qCritical() << "Ignoring" << versionID
						 << "because it doesn't have the version type set.";
			continue;
		}
		// OneSix or Legacy. use filter to determine type
		if (versionTypeStr == "release")
		{
		}
		else if (versionTypeStr == "snapshot") // It's a snapshot... yay
		{
		}
		else if (versionTypeStr == "old_alpha")
		{
		}
		else if (versionTypeStr == "old_beta")
		{
		}
		else
		{
			qCritical() << "Ignoring" << versionID
						 << "because it has an invalid version type.";
			continue;
		}
		mcVersion->m_type = versionTypeStr;
		qDebug() << "Loaded version" << versionID << "from"
					<< ((source == Remote) ? "remote" : "local") << "version list.";
		tempList.append(mcVersion);
	}
	updateListData(tempList);
	if(source == Remote)
	{
		m_loaded = true;
	}
}

void MinecraftVersionList::sort()
{
	beginResetModel();
	sortInternal();
	endResetModel();
}

QVariant MinecraftVersionList::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() > count())
		return QVariant();

	auto version = std::dynamic_pointer_cast<MinecraftVersion>(m_vlist[index.row()]);
	switch (role)
	{
	case VersionPointerRole:
		return qVariantFromValue(m_vlist[index.row()]);

	case VersionRole:
		return version->name();

	case VersionIdRole:
		return version->descriptor();

	case RecommendedRole:
		return version->descriptor() == m_latestReleaseID;

	case LatestRole:
	{
		if(version->descriptor() != m_latestSnapshotID)
			return false;
		MinecraftVersionPtr latestRelease = std::dynamic_pointer_cast<MinecraftVersion>(getLatestStable());
		/*
		if(latestRelease && latestRelease->m_releaseTime > version->m_releaseTime)
		{
			return false;
		}
		*/
		return true;
	}

	case TypeRole:
		return version->typeString();

	default:
		return QVariant();
	}
}

BaseVersionList::RoleList MinecraftVersionList::providesRoles()
{
	return {VersionPointerRole, VersionRole, VersionIdRole, RecommendedRole, LatestRole, TypeRole};
}

BaseVersionPtr MinecraftVersionList::getLatestStable() const
{
	if(m_lookup.contains(m_latestReleaseID))
		return m_lookup[m_latestReleaseID];
	return BaseVersionPtr();
}

BaseVersionPtr MinecraftVersionList::getRecommended() const
{
	return getLatestStable();
}

void MinecraftVersionList::updateListData(QList<BaseVersionPtr> versions)
{
	beginResetModel();
	for (auto version : versions)
	{
		auto descr = version->descriptor();

		if (!m_lookup.contains(descr))
		{
			m_lookup[version->descriptor()] = version;
			m_vlist.append(version);
			continue;
		}
		auto orig = std::dynamic_pointer_cast<MinecraftVersion>(m_lookup[descr]);
		auto added = std::dynamic_pointer_cast<MinecraftVersion>(version);
		// updateListData is called after Mojang list loads. those can be local or remote
		// remote comes always after local
		// any other options are ignored
		if (orig->m_versionSource != Local || added->m_versionSource != Remote)
		{
			continue;
		}
		// alright, it's an update. put it inside the original, for further processing.
		orig->upstreamUpdate = added;
	}
	sortInternal();
	endResetModel();
}

inline QDomElement getDomElementByTagName(QDomElement parent, QString tagname)
{
	QDomNodeList elementList = parent.elementsByTagName(tagname);
	if (elementList.count())
		return elementList.at(0).toElement();
	else
		return QDomElement();
}

MCVListLoadTask::MCVListLoadTask(MinecraftVersionList *vlist)
{
	m_list = vlist;
	m_currentStable = NULL;
	vlistReply = nullptr;
}

void MCVListLoadTask::executeTask()
{
	setStatus(tr("Loading instance version list..."));
	auto worker = ENV.qnam();
	vlistReply = worker->get(QNetworkRequest(
		QUrl("http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + "versions.json")));
	connect(vlistReply, SIGNAL(finished()), this, SLOT(list_downloaded()));
}

void MCVListLoadTask::list_downloaded()
{
	if (vlistReply->error() != QNetworkReply::NoError)
	{
		vlistReply->deleteLater();
		emitFailed("Failed to load Minecraft main version list" + vlistReply->errorString());
		return;
	}

	auto data = vlistReply->readAll();
	vlistReply->deleteLater();
	try
	{
		QJsonParseError jsonError;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
		if (jsonError.error != QJsonParseError::NoError)
		{
			throw ListLoadError(
				tr("Error parsing version list JSON: %1").arg(jsonError.errorString()));
		}
		m_list->loadMojangList(jsonDoc, Remote);
	}
	catch (Exception &e)
	{
		emitFailed(e.cause());
		return;
	}

	emitSucceeded();
	return;
}

MCVListVersionUpdateTask::MCVListVersionUpdateTask(MinecraftVersionList *vlist,
												   QString updatedVersion)
	: Task()
{
	m_list = vlist;
	versionToUpdate = updatedVersion;
}

void MCVListVersionUpdateTask::executeTask()
{
	QString urlstr = "http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + versionToUpdate + "/" +
					 versionToUpdate + ".json";
	auto job = new NetJob("Version index");
	job->addNetAction(ByteArrayDownload::make(QUrl(urlstr)));
	specificVersionDownloadJob.reset(job);
	connect(specificVersionDownloadJob.get(), SIGNAL(succeeded()), SLOT(json_downloaded()));
	connect(specificVersionDownloadJob.get(), SIGNAL(failed(QString)), SIGNAL(failed(QString)));
	connect(specificVersionDownloadJob.get(), SIGNAL(progress(qint64, qint64)),
			SIGNAL(progress(qint64, qint64)));
	specificVersionDownloadJob->start();
}

void MCVListVersionUpdateTask::json_downloaded()
{
	NetActionPtr DlJob = specificVersionDownloadJob->first();
	auto data = std::dynamic_pointer_cast<ByteArrayDownload>(DlJob)->m_data;
	specificVersionDownloadJob.reset();

	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);

	if (jsonError.error != QJsonParseError::NoError)
	{
		emitFailed(tr("The download version file is not valid."));
		return;
	}
	VersionFilePtr file;
	try
	{
		file = VersionFile::fromJson(jsonDoc, "net.minecraft.json", false);
	}
	catch (Exception &e)
	{
		emitFailed(tr("Couldn't process version file: %1").arg(e.cause()));
		return;
	}

	// Strip LWJGL from the version file. We use our own.
	ProfileUtils::removeLwjglFromPatch(file);

	// TODO: recognize and add LWJGL versions here.

	file->fileId = "net.minecraft";

	// now dump the file to disk
	auto doc = file->toJson(false);
	auto newdata = doc.toBinaryData();
	QString targetPath = "versions/" + versionToUpdate + "/" + versionToUpdate + ".dat";
	ensureFilePathExists(targetPath);
	QSaveFile vfile1(targetPath);
	if (!vfile1.open(QIODevice::Truncate | QIODevice::WriteOnly))
	{
		emitFailed(tr("Can't open %1 for writing.").arg(targetPath));
		return;
	}
	qint64 actual = 0;
	if ((actual = vfile1.write(newdata)) != newdata.size())
	{
		emitFailed(tr("Failed to write into %1. Written %2 out of %3.")
					   .arg(targetPath)
					   .arg(actual)
					   .arg(newdata.size()));
		return;
	}
	if (!vfile1.commit())
	{
		emitFailed(tr("Can't commit changes to %1").arg(targetPath));
		return;
	}

	m_list->finalizeUpdate(versionToUpdate);
	emitSucceeded();
}

std::shared_ptr<Task> MinecraftVersionList::createUpdateTask(QString version)
{
	return std::shared_ptr<Task>(new MCVListVersionUpdateTask(this, version));
}

void MinecraftVersionList::saveCachedList()
{
	// FIXME: throw.
	if (!ensureFilePathExists(localVersionCache))
		return;
	QSaveFile tfile(localVersionCache);
	if (!tfile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return;
	QJsonObject toplevel;
	QJsonArray entriesArr;
	for (auto version : m_vlist)
	{
		auto mcversion = std::dynamic_pointer_cast<MinecraftVersion>(version);
		// do not save the remote versions.
		if (mcversion->m_versionSource != Local)
			continue;
		QJsonObject entryObj;

		entryObj.insert("id", mcversion->descriptor());
		entryObj.insert("version", mcversion->descriptor());
		entryObj.insert("time", mcversion->m_updateTimeString);
		entryObj.insert("releaseTime", mcversion->m_releaseTimeString);
		entryObj.insert("type", mcversion->m_type);
		entriesArr.append(entryObj);
	}
	toplevel.insert("versions", entriesArr);

	{
		bool someLatest = false;
		QJsonObject latestObj;
		if(!m_latestReleaseID.isNull())
		{
			latestObj.insert("release", m_latestReleaseID);
			someLatest = true;
		}
		if(!m_latestSnapshotID.isNull())
		{
			latestObj.insert("snapshot", m_latestSnapshotID);
			someLatest = true;
		}
		if(someLatest)
		{
			toplevel.insert("latest", latestObj);
		}
	}

	QJsonDocument doc(toplevel);
	QByteArray jsonData = doc.toBinaryData();
	qint64 result = tfile.write(jsonData);
	if (result == -1)
		return;
	if (result != jsonData.size())
		return;
	tfile.commit();
}

void MinecraftVersionList::finalizeUpdate(QString version)
{
	int idx = -1;
	for (int i = 0; i < m_vlist.size(); i++)
	{
		if (version == m_vlist[i]->descriptor())
		{
			idx = i;
			break;
		}
	}
	if (idx == -1)
	{
		return;
	}

	auto updatedVersion = std::dynamic_pointer_cast<MinecraftVersion>(m_vlist[idx]);

	// reject any updates to builtin versions.
	if (updatedVersion->m_versionSource == Builtin)
		return;

	// if we have an update for the version, replace it, make the update local
	if (updatedVersion->upstreamUpdate)
	{
		auto updatedWith = updatedVersion->upstreamUpdate;
		updatedWith->m_versionSource = Local;
		m_vlist[idx] = updatedWith;
		m_lookup[version] = updatedWith;
	}
	else
	{
		// otherwise, just set the version as local;
		updatedVersion->m_versionSource = Local;
	}

	dataChanged(index(idx), index(idx));

	saveCachedList();
}

#include "MinecraftVersionList.moc"
