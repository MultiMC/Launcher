/* Copyright 2013-2015 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QObject>
#include <QList>
#include <QUrl>

#include "net/NetJob.h"
#include "tasks/Task.h"
#include "minecraft/VersionFilterData.h"
#include <quazip.h>

class OneSixInstance;
class SequentialTask;

class OneSixUpdate : public Task
{
	Q_OBJECT
public:
	explicit OneSixUpdate(OneSixInstance *inst, QObject *parent = 0);
	virtual void executeTask();

private
slots:
	void versionUpdateFailed(QString reason);
	void versionUpdateSucceeded();

private:
	/// the task that is spawned for version updates
	std::shared_ptr<SequentialTask> versionUpdateTask;

	/// the task that is spawned for fetching libraries and assets
	std::shared_ptr<Task> filesTask;

	OneSixInstance *m_inst = nullptr;
};
