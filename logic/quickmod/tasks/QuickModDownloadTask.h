#pragma once

#include "logic/tasks/Task.h"

class OneSixInstance;

class QuickModDownloadTask : public Task
{
	Q_OBJECT
public:
	explicit QuickModDownloadTask(OneSixInstance *instance, QObject *parent = 0);

protected:
	void executeTask();

private
slots:

private:
	OneSixInstance *m_instance;
};
