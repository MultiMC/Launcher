#pragma once

#include "Library.h"
#include "wonko/DownloadableResource.h"

class Task;

namespace Minecraft
{

class Libraries : public DownloadableResource
{
public:
	explicit Libraries(const QList<LibraryPtr> &libraries = {});

	void applyTo(const ResourcePtr &target) const override;
	Task *updateTask() const override;

	void clear()
	{
		shouldOverwriteLibs = false;
		overwriteLibs.clear();
		addLibs.clear();
		removeLibs.clear();
	}

	QList<LibraryPtr> getActiveLibs() const;

	/* DATA */
	bool shouldOverwriteLibs = false;
	QList<LibraryPtr> overwriteLibs;
	QList<LibraryPtr> addLibs;
	QList<QString> removeLibs;
};
class LibrariesFactory : public DownloadableResourceFactory
{
public:
	explicit LibrariesFactory(const int version, const QString &key) : DownloadableResourceFactory(version, key) {}

	ResourcePtr create(const int formatVersion, const QString &key, const QJsonValue &data) const override;
	DownloadPtr createDownload(const QJsonObject &obj) const override;
};
}
