#pragma once

// FIXME: merge with HttpMetaCache and make generic
#include <QString>
#include <memory>

class FileStore
{
public:
	FileStore(QString root);
	bool contains(const QString &key);
	bool store(const QString &key, const QByteArray &value);
	bool retrieve(const QString& key, QByteArray & value);
	bool remove(const QString &key);
private:
	QString m_root;
};

typedef std::shared_ptr<FileStore> FileStorePtr;