#pragma once

#include "logic/tasks/Task.h"

class OneSixInstance;

class QuickModForgeDownloadTask : public Task
{
	Q_OBJECT
public:
	QuickModForgeDownloadTask(OneSixInstance *instance, QObject *parent = 0);

protected:
	void executeTask();

private:
	OneSixInstance *m_instance;
};
