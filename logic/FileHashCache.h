#pragma once

#include <QString>
#include <QHash>

class QFileInfo;
class QJsonObject;

class FileHashCache
{
public:
	static FileHashCache *instance();

	QByteArray hash(const QFileInfo &file);
	QByteArray hash(const QString &file);
	QFileInfo reverseHash(const QByteArray &hash);

	struct Entry
	{
		explicit Entry() {}
		explicit Entry(const QFileInfo &info);
		QFileInfo toFileInfo() const;
		bool operator==(const Entry &other) const;
		QJsonObject toJson(const QByteArray &hash) const;

		QString path; // relative to MMC root
		int lastModifiedTimestamp;
	};

private:
	explicit FileHashCache() {}
	QHash<Entry, QByteArray> m_entryToHashMap;
	QHash<QByteArray, Entry> m_hashToEntryMap;

	void computeAndAdd(const Entry &entry);
};

inline uint qHash(const FileHashCache::Entry &entry) { return qHash(entry.path); }
