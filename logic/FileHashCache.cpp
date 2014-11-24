#include "FileHashCache.h"

#include <QFileInfo>
#include <QCryptographicHash>
#include <QDir>
#include <QDateTime>
#include <QJsonObject>

#include "logger/QsLog.h"

// TODO persistant storage

FileHashCache *FileHashCache::instance()
{
	static FileHashCache fhc;
	return &fhc;
}

QByteArray FileHashCache::hash(const QFileInfo &file)
{
	// don't hash directories...
	if (file.isDir())
	{
		return QByteArray();
	}

	const Entry entry(file);
	if (!m_entryToHashMap.contains(entry))
	{
		computeAndAdd(entry);
	}
	return m_entryToHashMap.value(entry);
}
QByteArray FileHashCache::hash(const QString &file)
{
	return hash(QFileInfo(QDir::current(), file));
}
QFileInfo FileHashCache::reverseHash(const QByteArray &hash)
{
	return m_hashToEntryMap.value(hash).toFileInfo();
}

void FileHashCache::computeAndAdd(const Entry &entry)
{
	QFile file(entry.toFileInfo().absoluteFilePath());
	if (!file.open(QFile::ReadOnly))
	{
		QLOG_ERROR() << "Unable to open " << entry.path << " for hashing";
		return;
	}
	QCryptographicHash hasher(QCryptographicHash::Sha1);
	hasher.addData(&file);
	file.close();
	const QByteArray hash = hasher.result();
	m_entryToHashMap.insert(entry, hash);
	m_hashToEntryMap.insert(hash, entry);
}

FileHashCache::Entry::Entry(const QFileInfo &info)
	: path(QDir::current().relativeFilePath(info.absoluteFilePath())),
	  lastModifiedTimestamp(info.lastModified().toMSecsSinceEpoch())
{
}
QFileInfo FileHashCache::Entry::toFileInfo() const
{
	return QFileInfo(QDir::current(), path);
}
bool FileHashCache::Entry::operator==(const FileHashCache::Entry &other) const
{
	return path == other.path && lastModifiedTimestamp == other.lastModifiedTimestamp;
}
QJsonObject FileHashCache::Entry::toJson(const QByteArray &hash) const
{
	QJsonObject obj;
	obj.insert("path", path);
	obj.insert("timestamp", lastModifiedTimestamp);
	obj.insert("hash", QString::fromUtf8(hash.toBase64()));
	return obj;
}
