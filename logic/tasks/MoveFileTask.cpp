// Licensed under the Apache-2.0 license. See README.md for details.

#include "MoveFileTask.h"

#include <QDir>
#include <pathutils.h>

MoveFileTask::MoveFileTask(const QString &from, const QString &to, QObject *parent)
	: Task(parent), m_from(from), m_to(to)
{
}
MoveFileTask::MoveFileTask(QObject *parent)
	: Task(parent)
{
}

void MoveFileTask::executeTask()
{
	if (QFile::exists(m_to))
	{
		deletePath(m_to);
	}
	ensureFilePathExists(m_to);
	QFile file(m_from);
	if (!file.rename(m_to))
	{
		emitFailed(tr("Unable to move to %1: %2").arg(m_to, file.errorString()));
	}
	else
	{
		emitSucceeded();
	}
}
