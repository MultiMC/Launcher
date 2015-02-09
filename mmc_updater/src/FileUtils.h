#pragma once

#include <exception>

#include <QString>

/** A set of functions for performing common operations
 * on files, throwing exceptions if an operation fails.
 *
 * Path arguments to FileUtils functions should use Unix-style path
 * separators.
 */
namespace FileUtils
{
/** Base class for exceptions reported by
 * FileUtils methods if an operation fails.
 */
class IOException : public std::exception
{
public:
	IOException(const QString &error);
	IOException(int errorCode, const QString &error);

	virtual ~IOException() throw();

	enum Type
	{
		NoError,
		/** Unknown error type.  Call what() to get the description
		 * provided by the OS.
		 */
		Unknown,
		ReadOnlyFileSystem,
		DiskFull
	};

	virtual const char *what() const throw()
	{
		return m_error.toLocal8Bit().constData();
	}

	Type type() const;

private:
	void init(int errorCode, const QString &error);

	QString m_error;
	int m_errorCode;
};

/** Remove a file.  Throws an exception if the file
 * could not be removed.
 *
 * On Unix, a file can be removed even if it is in use if the user
 * has the necessary permissions.  removeFile() tries to simulate
 * this behavior on Windows.  If a file cannot be removed on Windows
 * because it is in use it will be moved to a temporary directory and
 * scheduled for deletion on the next restart.
 */
void removeFile(const QString &src) throw(IOException);

void removeDir(const QString &path) throw(IOException);

/** Set the permissions of a file.  @p permissions uses the standard
 * Unix mode_t values.
 */
void chmod(const QString &path, int permissions) throw(IOException);

/** Returns true if the file at @p path exists.  If @p path is a symlink,
 * returns true if the symlink itself exists, not the target.
 */
bool fileExists(const QString &path) throw(IOException);

/** Returns the Unix mode flags of @p path.  If @p path is a symlink,
 * returns the mode flags of the target.
 */
int fileMode(const QString &path) throw(IOException);

void moveFile(const QString &src, const QString &dest) throw(IOException);
void touch(const QString &path) throw(IOException);
void copyFile(const QString &src, const QString &dest) throw(IOException);

/** Create all the directories in @p path which do not yet exist.
 * @p path may be relative or absolute.
 */
void mkpath(const QString &path) throw(IOException);

/** Returns the file name part of a file path, including the extension. */
QString fileName(const QString &path);

/** Returns the directory part of a file path.
 * On Windows this includes the drive letter, if present in @p path.
 */
QString dirname(const QString &path);

/** Return the full, absolute path to a file, resolving any
 * symlinks and removing redundant sections.
 */
QString canonicalPath(const QString &path);

/** Returns the path to a directory for storing temporary files. */
QString tempPath();

/** Converts @p path to an absolute path.  If @p path is already absolute,
 * just returns @p path, otherwise prefixes it with @p basePath to make it absolute.
 *
 * @p basePath should be absolute.
 */
QString makeAbsolute(const QString &path, const QString &basePath);

QString readFile(const QString &path) throw(IOException);
void writeFile(const QString &path, const QByteArray &contents) throw(IOException);
}
