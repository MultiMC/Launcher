#include "FileUtils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "Log.h"

#ifdef Q_OS_UNIX
// from <sys/stat.h>
#ifndef S_IRUSR
#define __S_IREAD 0400	 /* Read by owner.  */
#define __S_IWRITE 0200	/* Write by owner.  */
#define __S_IEXEC 0100	 /* Execute by owner.  */
#define S_IRUSR __S_IREAD  /* Read by owner.  */
#define S_IWUSR __S_IWRITE /* Write by owner.  */
#define S_IXUSR __S_IEXEC  /* Execute by owner.  */

#define S_IRGRP (S_IRUSR >> 3) /* Read by group.  */
#define S_IWGRP (S_IWUSR >> 3) /* Write by group.  */
#define S_IXGRP (S_IXUSR >> 3) /* Execute by group.  */

#define S_IROTH (S_IRGRP >> 3) /* Read by others.  */
#define S_IWOTH (S_IWGRP >> 3) /* Write by others.  */
#define S_IXOTH (S_IXGRP >> 3) /* Execute by others.  */
#endif
static QFile::Permissions unixModeToPermissions(const int mode)
{
	QFile::Permissions perms;

	if (mode & S_IRUSR)
	{
		perms |= QFile::ReadUser;
	}
	if (mode & S_IWUSR)
	{
		perms |= QFile::WriteUser;
	}
	if (mode & S_IXUSR)
	{
		perms |= QFile::ExeUser;
	}

	if (mode & S_IRGRP)
	{
		perms |= QFile::ReadGroup;
	}
	if (mode & S_IWGRP)
	{
		perms |= QFile::WriteGroup;
	}
	if (mode & S_IXGRP)
	{
		perms |= QFile::ExeGroup;
	}

	if (mode & S_IROTH)
	{
		perms |= QFile::ReadOther;
	}
	if (mode & S_IWOTH)
	{
		perms |= QFile::WriteOther;
	}
	if (mode & S_IXOTH)
	{
		perms |= QFile::ExeOther;
	}
	return perms;
}
#endif

FileUtils::IOException::IOException(const QString &error)
{
	init(errno, error);
}

FileUtils::IOException::IOException(int errorCode, const QString &error)
{
	init(errorCode, error);
}

void FileUtils::IOException::init(int errorCode, const QString &error)
{
	m_error = error;

#ifdef Q_OS_LINUX
	m_errorCode = errorCode;

	if (m_errorCode > 0)
	{
		m_error += " details: " + QString::fromLocal8Bit(strerror(m_errorCode));
	}
#endif

#ifdef Q_OS_WIN
	m_errorCode = 0;
	m_error += " GetLastError returned: " + QString::number(GetLastError());
#endif
}

FileUtils::IOException::~IOException() throw()
{
}

FileUtils::IOException::Type FileUtils::IOException::type() const
{
#ifdef Q_OS_LINUX
	switch (m_errorCode)
	{
	case 0:
		return NoError;
	case EROFS:
		return ReadOnlyFileSystem;
	case ENOSPC:
		return DiskFull;
	default:
		return Unknown;
	}
#else
	return Unknown;
#endif
}

bool FileUtils::fileExists(const QString &path) throw(IOException)
{
	return QFile::exists(path);
}

int FileUtils::fileMode(const QString &path) throw(IOException)
{
	return QFile::permissions(path);
}

void FileUtils::chmod(const QString &path, int mode) throw(IOException)
{
#ifdef Q_OS_LINUX
	if (!QFile::setPermissions(path, unixModeToPermissions(mode)))
	{
		throw IOException("Failed to set permissions on " + path + " to " +
						  QString::number(mode));
	}
#endif
}

void FileUtils::moveFile(const QString &src, const QString &dest) throw(IOException)
{
	if (!QFile::rename(src, dest))
	{
		throw IOException("Unable to move " + src + " to " + dest);
	}
}

void FileUtils::mkpath(const QString &dir) throw(IOException)
{
	if (!QDir().mkpath(dir))
	{
		throw IOException("Unable to create directory " + dir);
	}
}

void FileUtils::removeFile(const QString &src) throw(IOException)
{
	QFile file(src);
	if (!file.remove())
	{
		throw IOException("Unable to remove file " + src);
	}
}

QString FileUtils::fileName(const QString &path)
{
	return QFileInfo(path).fileName();
}

QString FileUtils::dirname(const QString &path)
{
	const QFileInfo info(path);
	if (info.isRelative())
	{
		const QString ret = QDir::current().relativeFilePath(info.absoluteDir().absolutePath());
		return ret.isEmpty() ? "." : ret;
	}
	else
	{
		return info.absoluteDir().absolutePath();
	}
}

void FileUtils::touch(const QString &path) throw(IOException)
{
	QFile file(path);
	if (!file.open(QFile::WriteOnly | QFile::Append))
	{
		throw IOException("Couldn't open a file for writing: " + file.errorString());
	}
	file.close();
}

QString FileUtils::canonicalPath(const QString &path)
{
	return QFileInfo(path).canonicalFilePath();
}

QString FileUtils::tempPath()
{
	return QDir::tempPath();
}

QString FileUtils::readFile(const QString &path) throw(IOException)
{
	QFile file(path);
	if (!file.open(QFile::ReadOnly))
	{
		throw IOException("Failed to read file " + path);
	}
	return QString::fromLatin1(file.readAll());
}

void FileUtils::copyFile(const QString &src, const QString &dest) throw(IOException)
{
	if (!QFile::copy(src, dest))
	{
		throw IOException("Failed to copy " + src + " to " + dest);
	}
	QFile::setPermissions(dest, QFile::permissions(src));
}

QString FileUtils::makeAbsolute(const QString &path, const QString &basePath)
{
	return QDir(basePath).absoluteFilePath(path);
}

void FileUtils::removeDir(const QString &path) throw(IOException)
{
	if (!QDir(path).removeRecursively())
	{
		throw IOException("Couldn't remove directory " + path);
	}
}

void FileUtils::writeFile(const QString &path, const QByteArray &contents) throw(IOException)
{
	QFile file(path);
	if (!file.open(QFile::WriteOnly))
	{
		throw IOException("Failed to write file " + path);
	}
	file.write(contents);
}
