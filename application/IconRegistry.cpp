// Licensed under the Apache-2.0 license. See README.md for details.

#include "IconRegistry.h"

#include <QDir>
#include <QMetaProperty>
#include <QLabel>
#include <QAction>
#include <QToolButton>
#include <QAbstractItemModel>
#include <QUrl>
#include <QPersistentModelIndex>

#include "net/CacheDownload.h"
#include "net/NetJob.h"
#include "net/HttpMetaCache.h"
#include "Env.h"
#include "MultiMC.h"

/// Fetches an icon and updates a model index
class ModelIconFetcher : public QObject
{
	Q_OBJECT
public:
	explicit ModelIconFetcher(const QPersistentModelIndex &index, const QUrl &url, QObject *parent = nullptr)
		: QObject(parent)
	{
		auto entry = ENV.metacache()->resolveEntry("icons", url.toString());
		CacheDownloadPtr ptr = CacheDownload::make(url, entry);
		NetJob *job = new NetJob("Icon download");
		job->addNetAction(ptr);
		connect(job, &NetJob::succeeded, this, [this, index]()
		{
			if (index.isValid())
			{
				emit const_cast<QAbstractItemModel *>(index.model())->dataChanged(index, index, QVector<int>() << Qt::DecorationRole);
			}
			MMC->iconRegistry()->m_modelFetchers.remove(index);
			deleteLater();
		});
		connect(job, &NetJob::failed, this, &ModelIconFetcher::deleteLater);
		QMetaObject::invokeMethod(job, "start", Qt::QueuedConnection);
	}
};
/// Fetches an icon and updates a target
class TargetIconFetcher : public QObject
{
	Q_OBJECT
public:
	explicit TargetIconFetcher(const IconRegistry::Target &target, const QUrl &url, QObject *parent = nullptr)
		: QObject(parent)
	{
		auto entry = ENV.metacache()->resolveEntry("icons", url.toString());
		CacheDownloadPtr ptr = CacheDownload::make(url, entry);
		NetJob *job = new NetJob("Icon download");
		job->addNetAction(ptr);
		connect(job, &NetJob::succeeded, this, [this, target]()
		{
			if (!target.object.isNull() && MMC->iconRegistry()->m_targetFetchers.contains(target))
			{
				MMC->iconRegistry()->updateTarget(target);
			}
			MMC->iconRegistry()->m_targetFetchers.remove(target);
		});
		connect(job, &NetJob::failed, this, &TargetIconFetcher::deleteLater);
		QMetaObject::invokeMethod(job, "start", Qt::QueuedConnection);
	}
};

IconRegistry::IconRegistry()
{
}

QIcon IconRegistry::icon(const QString &key) const
{
	// first case: local file with an absolute path
	const QFileInfo fileinfo(key);
	if (fileinfo.exists() && fileinfo.isAbsolute())
	{
		return QIcon(key);
	}

	return MMC->getThemedIcon(key);

	const QStringList files = findFiles(QString(key).remove(".png"));
	QIcon icon;
	for (const auto file : files)
	{
		bool ok = false;
		const int resolution = QFileInfo(file).baseName().toInt(&ok);
		if (ok)
		{
			icon.addFile(file, QSize(resolution, resolution));
		}
		else
		{
			icon.addFile(file);
		}
	}
	return icon;
}
QIcon IconRegistry::icon(const QString &key, const QModelIndex &index) const
{
	const QUrl url(key);
	if (url.isValid() && !url.isRelative())
	{
		// check if we need to (re)fetch the icon
		auto entry = ENV.metacache()->resolveEntry("icons", url.toString());
		if (entry->stale && !m_modelFetchers.contains(index))
		{
			m_modelFetchers.insert(index, new ModelIconFetcher(index, url));
		}
		return icon(entry->getFullPath());
	}
	else
	{
		return MMC->getThemedIcon(key);
		return icon(key);
	}
}
QPixmap IconRegistry::pixmap(const QString &key) const
{
	// first case: local file with an absolute path
	const QFileInfo fileinfo(key);
	if (fileinfo.exists() && fileinfo.isAbsolute())
	{
		return QPixmap(key);
	}

	return MMC->getThemedIcon(key).pixmap(256);

	const QStringList files = findFiles(QString(key).remove(".png"));
	if (files.size() == 1)
	{
		// early exit for when only a single file is available
		return QPixmap(files.first());
	}
	else if (files.isEmpty())
	{
		// early exit for when no files are available
		return QPixmap();
	}

	// sort all icon sizes so that we can return the biggest
	QList<QPair<int, QString>> sizes;
	for (const auto file : files)
	{
		bool ok = false;
		const int size = QFileInfo(file).baseName().toInt(&ok);
		if (ok)
		{
			sizes.append(qMakePair(size, file));
		}
	}
	if (sizes.isEmpty())
	{
		// couldn't find any valid sizes: pick a file at random
		return QPixmap(files.first());
	}
	std::sort(sizes.begin(), sizes.end(), [](const QPair<int, QString> &a, const QPair<int, QString> &b) { return a.first < b.first; });
	return QPixmap(files.first());
}

QStringList IconRegistry::keys(const QString &base) const
{
	return QDir(":/" + base).entryList(QDir::Files);
}

void IconRegistry::setTheme(const QString &theme)
{
	m_theme = theme;

	// update targets
	for (const auto target : m_targets.keys())
	{
		updateTarget(target);
	}
	for (const auto pair : m_targetModels)
	{
		QAbstractItemModel *model = pair.first;
		emit model->dataChanged(model->index(0, pair.second), model->index(model->rowCount(), pair.second), QVector<int>() << Qt::DecorationRole);
	}
}
QString IconRegistry::theme() const
{
	return m_theme;
}

void IconRegistry::setForTarget(QObject *target, const char *property, const QString &key, const int size)
{
	auto t = Target{target, property, size};
	m_targets[t] = key;
	updateTarget(t);

	// still update the value so that it gets emptied
	if (key.isEmpty())
	{
		m_targets.remove(t);
	}
}
void IconRegistry::setForTarget(QLabel *label, const QString &key, const int size)
{
	setForTarget(label, "pixmap", key, size);
}
void IconRegistry::setForTarget(QAction *action, const QString &key, const int size)
{
	setForTarget(action, "icon", key, size);
}
void IconRegistry::setForTarget(QAbstractButton *button, const QString &key, const int size)
{
	setForTarget(button, "icon", key, size);
}

void IconRegistry::setForModel(QAbstractItemModel *model, const int column)
{
	m_targetModels.append(qMakePair(model, column));
}

void IconRegistry::updateTarget(const IconRegistry::Target &target)
{
	Q_ASSERT(m_targets.contains(target));

	QString key = m_targets[target];
	const QUrl url(key);
	if (url.isValid() && !url.isRelative())
	{
		// check if we need to (re)fetch the icon
		auto entry = ENV.metacache()->resolveEntry("icons", url.toString());
		if (entry->stale && !m_targetFetchers.contains(target))
		{
			m_targetFetchers.insert(target, new TargetIconFetcher(target, url));
		}
		// set the icon to something even if we don't have an icon, this function will be called again when the skin has arrived
		key = entry->getFullPath();
	}

	const QMetaProperty prop = target.object->metaObject()->property(target.object->metaObject()->indexOfProperty(target.property));
	if (target.size > 0)
	{
		prop.write(target.object, icon(key).pixmap(target.size));
	}
	else if (prop.type() == QVariant::Icon)
	{
		prop.write(target.object, icon(key));
	}
	else
	{
		prop.write(target.object, pixmap(key));
	}
}

QStringList IconRegistry::findFiles(const QString &key) const
{
	QStringList files;

	// first case: for the current theme
	for (const auto entry : QDir(":/" + m_theme + "/" + key).entryInfoList(QDir::Files))
	{
		files += entry.absoluteFilePath();
	}
	if (!files.isEmpty())
	{
		return files;
	}

	// second case: the default fallback theme; "icons"
	for (const auto entry : QDir(":/icons/" + key).entryInfoList(QDir::Files))
	{
		files += entry.absoluteFilePath();
	}
	if (!files.isEmpty())
	{
		return files;
	}

	// third case: full path
	if (!QFile::exists(":/" + key + ".png"))
	{
		qWarning("Requested icon \"%s\" doesn't exist", qPrintable(key));
	}
	return QStringList() << ":/" + key + ".png";
}

#include "IconRegistry.moc"
