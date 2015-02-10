#include "FileStore.h"
#include <pathutils.h>
#include <QSaveFile>

FileStore::FileStore(QString root)
{
	m_root = root;
}

bool FileStore::contains(const QString& key)
{
	return QFile::exists(PathCombine(m_root, key));
}

bool FileStore::remove(const QString& key)
{
	auto path = PathCombine(m_root, key);
	if(QFile::exists(path))
		return true;
	return QFile::remove(path);
}

bool FileStore::retrieve(const QString& key, QByteArray & value)
{
	QFile foo(PathCombine(m_root, key));
	if(!foo.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
	{
		return false;
	}
	value = foo.readAll();
	return true;
}

bool FileStore::store(const QString& key, const QByteArray& value)
{
	auto path = PathCombine(m_root, key);
	if(!ensureFilePathExists(path))
	{
		return false;
	}
	QSaveFile foo(path);
	if(!foo.open(QIODevice::WriteOnly | QIODevice::Unbuffered))
	{
		return false;
	}
	if(foo.write(value) != value.size())
	{
		foo.cancelWriting();
		return false;
	}
	if(!foo.commit())
	{
		return false;
	}
	return true;
}
