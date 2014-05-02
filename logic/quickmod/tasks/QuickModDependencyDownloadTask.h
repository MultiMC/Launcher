#pragma once

#include "logic/tasks/Task.h"
#include "logic/quickmod/QuickMod.h"

class QuickModDependencyDownloadTask : public Task
{
	Q_OBJECT
public:
	explicit QuickModDependencyDownloadTask(QList<QuickModUid> mods, QObject *parent = 0);

protected:
	void executeTask();

private
slots:
	void modAdded(QuickModPtr mod);

private:
	QList<QuickModUid> m_mods;
	// list of mods we are still waiting for
	QList<QuickModUid> m_pendingMods;
	// list of mods we have requested
	QList<QuickModUid> m_requestedMods;

	int m_lastSetPercentage;
	void updateProgress();

	void requestDependenciesOf(const QuickModPtr mod);
};
