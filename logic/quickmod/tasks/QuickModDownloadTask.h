#pragma once

#include "logic/tasks/Task.h"

#include <memory>

class OneSixInstance;
class BaseInstance;
typedef std::shared_ptr<BaseInstance> InstancePtr;

class QuickModDownloadTask : public Task
{
	Q_OBJECT
public:
	explicit QuickModDownloadTask(InstancePtr instance, QObject *parent = 0);

protected:
	void executeTask();

private
slots:

private:
	InstancePtr m_instance;
};
