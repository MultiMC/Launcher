// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include "Task.h"

class CopyFileTask : public Task
{
	Q_OBJECT
public:
	explicit CopyFileTask(const QString &from, const QString &to, QObject *parent = nullptr);
	explicit CopyFileTask(QObject *parent = nullptr);

	void setFrom(const QString &from) { m_from = from; }
	QString from() const { return m_from; }

	void setTo(const QString &to) { m_to = to; }
	QString to() const { return m_to; }

private:
	void executeTask() override;

	QString m_from;
	QString m_to;
};
