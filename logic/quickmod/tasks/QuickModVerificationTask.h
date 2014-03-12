#pragma once

#include "logic/tasks/Task.h"

#include <memory>

#include "logic/net/NetJob.h"

class QuickMod;
class QuickModVersion;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;

class QuickModVerificationTask : public Task
{
	Q_OBJECT
public:
	explicit QuickModVerificationTask(const QList<QuickModVersionPtr> &modVersions, QObject *parent = 0);

protected:
	void executeTask();

private
slots:
	void downloadSucceeded(int index);
	void downloadFailure(int index);

private:
	QList<QuickModVersionPtr> m_modVersions;
	NetJobPtr m_netJob;
};
