#pragma once

#include "logic/tasks/Task.h"

class QuickMod;

class QuickModDependencyDownloadTask : public Task
{
	Q_OBJECT
public:
	explicit QuickModDependencyDownloadTask(QList<QuickMod *> mods, QObject *parent = 0);

protected:
	void executeTask();

private
slots:
	void modAdded(QuickMod *mod);

private:
	QList<QuickMod *> m_mods;
	// list of mods we are still waiting for
	QList<QUrl> m_pendingMods;
	// list of mods we have requested
	QList<QUrl> m_requestedMods;

	int m_lastSetPercentage;
	void updateProgress();

	void requestDependenciesOf(const QuickMod *mod);
};
