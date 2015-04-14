// Licensed under the Apache-2.0 license. See README.md for details.

#include "CopyFileTask.h"

#include <QDir>

#include <pathutils.h>
#include "Exception.h"

CopyFileTask::CopyFileTask(const QString &from, const QString &to, QObject *parent)
	: Task(parent), m_from(from), m_to(to)
{
}
CopyFileTask::CopyFileTask(QObject *parent)
	: Task(parent)
{
}

static QStringList gatherFiles(const QDir &dir, const QDir &root)
{
	QStringList out;
	for (const QFileInfo &entry : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs))
	{
		out += root.relativeFilePath(entry.absoluteFilePath());
		if (entry.isDir())
		{
			out += gatherFiles(entry.absoluteFilePath(), root);
		}
	}
	return out;
}
static void copyFile(const QString &src, const QString &dest)
{
	QFile file(src);
	if (!file.copy(dest))
	{
		throw Exception(QObject::tr("Unable to copy file to %1: %2").arg(dest, file.errorString()));
	}
}

void CopyFileTask::executeTask()
{
	if (!QFile::exists(m_to))
	{
		if (QFileInfo(m_from).isFile())
		{
			ensureFilePathExists(m_to);
			copyFile(m_from, m_to);
		}
		else
		{
			const QStringList files = gatherFiles(m_from, m_from);
			const int total = files.size();
			int current = 0;
			for (const auto file : files)
			{
				setProgress(current++ / total);

				const QFileInfo info(m_from, file);
				if (info.isDir())
				{
					ensureFolderPathExists(info.absoluteFilePath());
				}
				else
				{
					copyFile(info.absoluteFilePath(), QDir(m_to).absoluteFilePath(file));
				}
			}
		}
	}
	emitSucceeded();
}
