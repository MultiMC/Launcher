#pragma once

#include "logic/tasks/Task.h"

#include <memory>

class BaseInstance;
typedef std::shared_ptr<BaseInstance> InstancePtr;

class QuickModForgeDownloadTask : public Task
{
	Q_OBJECT
public:
	QuickModForgeDownloadTask(InstancePtr instance, QObject *parent = 0);

protected:
	void executeTask();

private:
	InstancePtr m_instance;
};
