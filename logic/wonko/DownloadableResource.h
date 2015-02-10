#pragma once

#include <QUrl>

#include "BaseResource.h"

class QJsonObject;
using NetActionPtr = std::shared_ptr<class NetAction>;
using DownloadPtr = std::shared_ptr<class BaseDownload>;

class BaseDownload
{
public:
	explicit BaseDownload(const QUrl &url, const int size, const QByteArray &hash)
		: m_url(url), m_size(size), m_hash(hash) {}
	virtual ~BaseDownload()
	{
	}

	virtual QUrl url() const
	{
		return m_url;
	}
	int size() const
	{
		return m_size;
	}
	QByteArray sha256() const
	{
		return m_hash;
	}

	virtual QList<NetActionPtr> createNetActions() const;

private:
	QUrl m_url;
	int m_size;
	QByteArray m_hash;
};

class DownloadableResource : public BaseResource
{
public:
	explicit DownloadableResource(const QList<DownloadPtr> &downloads)
		: m_downloads(downloads) {}
	virtual ~DownloadableResource()
	{
	}

	virtual Task *updateTask() const override;
	virtual void applyTo(const ResourcePtr &target) const override;

	QList<DownloadPtr> downloads() const
	{
		return m_downloads;
	}

private:
	QList<DownloadPtr> m_downloads;
};
class DownloadableResourceFactory : public StandardResourceFactory
{
public:
	explicit DownloadableResourceFactory(const int version, const QString &key) : StandardResourceFactory(version, key) {}

	ResourcePtr create(const int formatVersion, const QString &key, const QJsonValue &data) const override;
	virtual DownloadPtr createDownload(const QJsonObject &obj) const;
};
