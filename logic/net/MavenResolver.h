#pragma once

#include "logic/tasks/Task.h"
#include "logic/net/CacheDownload.h"

class OneSixInstance;

class MavenResolver : public Task
{
	Q_OBJECT
public:
	MavenResolver(std::shared_ptr<OneSixInstance> instance, Bindable *parent = nullptr);

	struct LibraryIdentifier
	{
		LibraryIdentifier()
		{
		}
		LibraryIdentifier(const QString &group, const QString &artifact, const QString &version)
			: group(group), artifact(artifact), version(version)
		{
		}

		QString group;
		QString artifact;
		QString version;
	};

protected:
	void executeTask();

private
slots:
	void downloadSucceeded(int index);
	void downloadFailed(int index);

private:
	std::shared_ptr<OneSixInstance> m_instance;

	QMap<int, QPair<CacheDownloadPtr, QUrl>> m_downloads;
	int getNextDownloadIndex() const;

	int m_lastSetPercentage;
	void updateProgress();

	void requestDependenciesOf(const LibraryIdentifier mod, const QUrl &url);
};
