#include "Assets.h"

#include <QDir>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>
#include <QDebug>

#include <pathutils.h>
#include "net/NetJob.h"
#include "net/URLConstants.h"
#include "tasks/Task.h"
#include "Json.h"
#include "Env.h"

namespace Minecraft {

struct AssetObject
{
	QString hash;
	qint64 size;
};

struct AssetsIndex
{
	QMap<QString, AssetObject> objects;
	bool isVirtual = false;
};

/*
 * Returns true on success, with index populated
 * index is undefined otherwise
 */
bool loadAssetsIndexJson(QString path, AssetsIndex *index)
{
	/*
	{
	  "objects": {
		"icons/icon_16x16.png": {
		  "hash": "bdf48ef6b5d0d23bbb02e17d04865216179f510a",
		  "size": 3665
		},
		...
		}
	  }
	}
	*/

	QFile file(path);

	// Try to open the file and fail if we can't.
	// TODO: We should probably report this error to the user.
	if (!file.open(QIODevice::ReadOnly))
	{
		qCritical() << "Failed to read assets index file" << path;
		return false;
	}

	// Read the file and close it.
	QByteArray jsonData = file.readAll();
	file.close();

	QJsonParseError parseError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

	// Fail if the JSON is invalid.
	if (parseError.error != QJsonParseError::NoError)
	{
		qCritical() << "Failed to parse assets index file:" << parseError.errorString()
					 << "at offset " << QString::number(parseError.offset);
		return false;
	}

	// Make sure the root is an object.
	if (!jsonDoc.isObject())
	{
		qCritical() << "Invalid assets index JSON: Root should be an array.";
		return false;
	}

	QJsonObject root = jsonDoc.object();

	QJsonValue isVirtual = root.value("virtual");
	if (!isVirtual.isUndefined())
	{
		index->isVirtual = isVirtual.toBool(false);
	}

	QJsonValue objects = root.value("objects");
	QVariantMap map = objects.toVariant().toMap();

	for (QVariantMap::const_iterator iter = map.begin(); iter != map.end(); ++iter)
	{
		// qDebug() << iter.key();

		QVariant variant = iter.value();
		QVariantMap nested_objects = variant.toMap();

		AssetObject object;

		for (QVariantMap::const_iterator nested_iter = nested_objects.begin();
			 nested_iter != nested_objects.end(); ++nested_iter)
		{
			// qDebug() << nested_iter.key() << nested_iter.value().toString();
			QString key = nested_iter.key();
			QVariant value = nested_iter.value();

			if (key == "hash")
			{
				object.hash = value.toString();
			}
			else if (key == "size")
			{
				object.size = value.toDouble();
			}
		}

		index->objects.insert(iter.key(), object);
	}

	return true;
}

QDir reconstructAssets(QString assetsId)
{
	QDir assetsDir = QDir("assets/");
	QDir indexDir = QDir(PathCombine(assetsDir.path(), "indexes"));
	QDir objectDir = QDir(PathCombine(assetsDir.path(), "objects"));
	QDir virtualDir = QDir(PathCombine(assetsDir.path(), "virtual"));

	QString indexPath = PathCombine(indexDir.path(), assetsId + ".json");
	QFile indexFile(indexPath);
	QDir virtualRoot(PathCombine(virtualDir.path(), assetsId));

	if (!indexFile.exists())
	{
		qCritical() << "No assets index file" << indexPath << "; can't reconstruct assets";
		return virtualRoot;
	}

	qDebug() << "reconstructAssets" << assetsDir.path() << indexDir.path()
				 << objectDir.path() << virtualDir.path() << virtualRoot.path();

	AssetsIndex index;
	bool loadAssetsIndex = loadAssetsIndexJson(indexPath, &index);

	if (loadAssetsIndex && index.isVirtual)
	{
		qDebug() << "Reconstructing virtual assets folder at" << virtualRoot.path();

		for (QString map : index.objects.keys())
		{
			AssetObject asset_object = index.objects.value(map);
			QString target_path = PathCombine(virtualRoot.path(), map);
			QFile target(target_path);

			QString tlk = asset_object.hash.left(2);

			QString original_path =
				PathCombine(PathCombine(objectDir.path(), tlk), asset_object.hash);
			QFile original(original_path);
			if (!original.exists())
				continue;
			if (!target.exists())
			{
				QFileInfo info(target_path);
				QDir target_dir = info.dir();
				// qDebug() << target_dir;
				if (!target_dir.exists())
					QDir("").mkpath(target_dir.path());

				bool couldCopy = original.copy(target_path);
				qDebug() << " Copying" << original_path << "to" << target_path
							 << QString::number(couldCopy); // << original.errorString();
			}
		}

		// TODO: Write last used time to virtualRoot/.lastused
	}

	return virtualRoot;
}

class AssetsUpdate : public Task
{
	Q_OBJECT
public:
	explicit AssetsUpdate(const Assets *assets, QObject *parent = 0) : Task(parent), m_assets(assets) {}
	virtual void executeTask();

private
slots:
	void assetIndexStart();
	void assetIndexFinished();
	void assetIndexFailed(QString error);

	void assetsFinished();
	void assetsFailed(QString error);

private:
	NetJobPtr assetDlJob;
	const Assets *m_assets;
};

void AssetsUpdate::executeTask()
{
	if(!m_assets->id().isEmpty())
	{
		assetIndexStart();
	}
}

void AssetsUpdate::assetIndexStart()
{
	setStatus(tr("Updating assets index..."));
	QUrl indexUrl = "http://" + URLConstants::AWS_DOWNLOAD_INDEXES + m_assets->id() + ".json";
	QString localPath = m_assets->id() + ".json";
	auto job = new NetJob(tr("Asset index for %1").arg(m_assets->id()));

	auto metacache = ENV.metacache();
	auto entry = metacache->resolveEntry("asset_indexes", localPath);
	job->addNetAction(CacheDownload::make(indexUrl, entry));
	assetDlJob.reset(job);

	connect(assetDlJob.get(), SIGNAL(succeeded()), SLOT(assetIndexFinished()));
	connect(assetDlJob.get(), SIGNAL(failed(QString)), SLOT(assetIndexFailed(QString)));
	connect(assetDlJob.get(), SIGNAL(progress(qint64, qint64)),
			SIGNAL(progress(qint64, qint64)));

	assetDlJob->start();
}

void AssetsUpdate::assetIndexFinished()
{
	AssetsIndex index;

	QString asset_fname = "assets/indexes/" + m_assets->id() + ".json";
	if (!loadAssetsIndexJson(asset_fname, &index))
	{
		auto metacache = ENV.metacache();
		auto entry = metacache->resolveEntry("asset_indexes", m_assets->id() + ".json");
		metacache->evictEntry(entry);
		emitFailed(tr("Failed to read the assets index!"));
	}

	QList<Md5EtagDownloadPtr> dls;
	for (auto object : index.objects.values())
	{
		QString objectName = object.hash.left(2) + "/" + object.hash;
		QFileInfo objectFile("assets/objects/" + objectName);
		if ((!objectFile.isFile()) || (objectFile.size() != object.size))
		{
			auto objectDL = MD5EtagDownload::make(
				QUrl("http://" + URLConstants::RESOURCE_BASE + objectName),
				objectFile.filePath());
			objectDL->m_total_progress = object.size;
			dls.append(objectDL);
		}
	}
	if (dls.size())
	{
		setStatus(tr("Getting the assets files from Mojang..."));
		auto job = new NetJob(tr("Assets download task for %1").arg(m_assets->id()));
		for (auto dl : dls)
			job->addNetAction(dl);
		assetDlJob.reset(job);
		connect(assetDlJob.get(), SIGNAL(succeeded()), SLOT(assetsFinished()));
		connect(assetDlJob.get(), SIGNAL(failed(QString)), SLOT(assetsFailed(QString)));
		connect(assetDlJob.get(), SIGNAL(progress(qint64, qint64)),
				SIGNAL(progress(qint64, qint64)));
		assetDlJob->start();
		return;
	}
	assetsFinished();
}

void AssetsUpdate::assetIndexFailed(QString error)
{
	emitFailed(tr("Failed to download the assets index: %1!").arg(error));
}

void AssetsUpdate::assetsFinished()
{
	emitSucceeded();
}

void AssetsUpdate::assetsFailed(QString error)
{
	emitFailed(tr("Failed to download assets: %1!").arg(error));
}

Task *Assets::prelaunchTask() const
{
	return nullptr;
}
Task *Assets::updateTask() const
{
	return new AssetsUpdate(this);
}
QString Assets::storageFolder()
{
	return reconstructAssets(id()).absolutePath();
}

ResourcePtr AssetsFactory::create(const int formatVersion, const QString &key, const QJsonValue &data) const
{
	return std::make_shared<Assets>(Json::ensureString(data));
}

#include "Assets.moc"

}
