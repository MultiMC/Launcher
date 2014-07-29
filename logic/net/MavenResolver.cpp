#include "MavenResolver.h"

#include <QXmlStreamReader>
#include <QtMath>

#include "logic/OneSixInstance.h"
#include "logic/net/HttpMetaCache.h"
#include "MultiMC.h"

struct POM : MavenResolver::LibraryIdentifier
{
	POM()
	{
	}
	POM(QIODevice *data)
	{
		QXmlStreamReader reader(data);
		while (!reader.atEnd())
		{
			if (reader.readNextStartElement())
			{
				const QString name = reader.name().toString();
				if (name == "groupId")
				{
					group = reader.readElementText();
				}
				else if (name == "artifactId")
				{
					artifact = reader.readElementText();
				}
				else if (name == "version")
				{
					version = reader.readElementText();
				}
				else if (name == "dependency")
				{
					dependencies.append(Dependency(reader));
				}
			}
		}
	}

	struct Dependency : MavenResolver::LibraryIdentifier
	{
		Dependency()
		{
		}
		Dependency(QXmlStreamReader &reader)
		{
			while (true)
			{
				switch (reader.readNext())
				{
				case QXmlStreamReader::StartElement:
				{
					const QString name = reader.name().toString();
					if (name == "groupId")
					{
						group = reader.readElementText();
					}
					else if (name == "artifactId")
					{
						artifact = reader.readElementText();
					}
					else if (name == "version")
					{
						version = reader.readElementText();
					}
					else if (name == "scope")
					{
						scope = reader.readElementText();
					}
				}
					break;
				case QXmlStreamReader::EndElement:
					if (reader.name() == "dependency")
					{
						return;
					}
				}
			}
		}

		QString scope;
	};
	QList<Dependency> dependencies;
};

MavenResolver::MavenResolver(InstancePtr instance, Bindable *parent)
	: Task(parent), m_instance(instance)
{
}

void MavenResolver::executeTask()
{
	m_downloads.clear();
	m_lastSetPercentage = 0;

	setStatus(tr("Fetching Maven POM files..."));

	auto libs = std::dynamic_pointer_cast<OneSixInstance>(m_instance)->getFullVersion()->libraries;
	for (auto lib : libs)
	{
		if (lib->hint() == "recurse")
		{
			const auto gradleSpec = lib->rawName();
			requestDependenciesOf(LibraryIdentifier(gradleSpec.groupId(), gradleSpec.artifactId(), gradleSpec.version()), lib->m_base_url);
		}
	}
	if (m_downloads.isEmpty())
	{
		emitSucceeded();
	}
	updateProgress();
}

void MavenResolver::downloadSucceeded(int index)
{
	QFile file(m_downloads[index].first->getTargetFilepath());
	if (!file.open(QFile::ReadOnly))
	{
		QLOG_ERROR() << "Couldn't open" << file.fileName() << ":" << file.errorString();
		emitFailed(tr("Couldn't open downloaded file"));
		return;
	}
	POM pom(&file);
	for (auto dep : pom.dependencies)
	{
		requestDependenciesOf(dep, m_downloads[index].second);
	}
	m_downloads.remove(index);
	updateProgress();
}
void MavenResolver::downloadFailed(int index)
{
	emitFailed(tr("Maven POM download failed"));
}

int MavenResolver::getNextDownloadIndex() const
{
	return m_downloads.isEmpty() ? 0 : m_downloads.keys().last() + 1;
}

void MavenResolver::updateProgress()
{
	int max = getNextDownloadIndex();
	int current = max - m_downloads.size();
	double percentage = (double)current * 100.0f / (double)max;
	m_lastSetPercentage = qMax(m_lastSetPercentage, qCeil(percentage));
	setProgress(m_lastSetPercentage);
	if (m_downloads.isEmpty())
	{
		emitSucceeded();
	}
}

void MavenResolver::requestDependenciesOf(const LibraryIdentifier lib, const QUrl &url)
{
	QString pom = QString("%1/%2/%3/%2-%3.pom").arg(QString(lib.group).replace('.', '/'), lib.artifact, lib.version);
	auto entry = MMC->metacache()->resolveEntry("maven/pom", pom);
	CacheDownloadPtr download = CacheDownload::make(url.resolved(QUrl(pom)), entry);
	m_downloads[getNextDownloadIndex()] = qMakePair(download, url);
	connect(download.get(), &CacheDownload::succeeded, this, &MavenResolver::downloadSucceeded);
	connect(download.get(), &CacheDownload::failed, this, &MavenResolver::downloadFailed);
	download->start();
	QLOG_INFO() << "Fetching dependencies of" << QString("%1:%2:%3").arg(lib.group, lib.artifact, lib.version);
}
