#include "DownloadableResource.h"

#include "net/CacheDownload.h"
#include "net/NetJob.h"
#include "net/HttpMetaCache.h"
#include "Env.h"
#include "Json.h"

QList<NetActionPtr> BaseDownload::createNetActions() const
{
	MetaEntryPtr ptr =
		ENV.metacache()->resolveEntry("cache", "general/" + m_hash.left(2) + '/' + m_hash);
	return QList<NetActionPtr>() << CacheDownload::make(url(), ptr);
}

Task *DownloadableResource::updateTask() const
{
	NetJob *job = new NetJob("Download resources");
	for (const DownloadPtr dl : m_downloads)
	{
		for (NetActionPtr ptr : dl->createNetActions())
		{
			job->addNetAction(ptr);
		}
	}
	return job;
}

void DownloadableResource::applyTo(const ResourcePtr &target) const
{
	QHash<QByteArray, DownloadPtr> downloads;
	for (DownloadPtr dl : m_downloads)
	{
		downloads.insert(dl->sha256(), dl);
	}

	for (DownloadPtr dl : std::dynamic_pointer_cast<DownloadableResource>(target)->m_downloads)
	{
		downloads.insert(dl->sha256(), dl);
	}

	std::dynamic_pointer_cast<DownloadableResource>(target)->m_downloads = downloads.values();
}


ResourcePtr DownloadableResourceFactory::create(const int formatVersion, const QString &key, const QJsonValue &data) const
{
	QList<DownloadPtr> downloads;
	for (const QJsonObject &obj : Json::ensureIsArrayOf<QJsonObject>(data))
	{
		downloads.append(createDownload(obj));
	}
	return std::make_shared<DownloadableResource>(downloads);
}

DownloadPtr DownloadableResourceFactory::createDownload(const QJsonObject &obj) const
{
	using namespace Json;
	return std::make_shared<BaseDownload>(ensureUrl(obj, "url"), ensureInteger(obj, "size"), ensureByteArray(obj, "sha256"));
}
