/* Copyright 2013 MultiMC Contributors
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

#include "logic/net/NetJob.h"
#include "logic/tasks/Task.h"

class MinecraftVersion;
class OneSixInstance;

class OneSixUpdate : public Task
{
	Q_OBJECT
public:
	explicit OneSixUpdate(OneSixInstance *inst, QObject *parent = 0);
	virtual void executeTask();

private
slots:
	void versionFileStart();
	void versionFileFinished();
	void versionFileFailed();

	void jarlibStart();
	void jarlibFinished();
	void jarlibFailed();

	void assetIndexStart();
	void assetIndexFinished();
	void assetIndexFailed();

	void assetsFinished();
	void assetsFailed();

private:
	NetJobPtr specificVersionDownloadJob;
	NetJobPtr jarlibDownloadJob;

	// target version, determined during this task
	std::shared_ptr<MinecraftVersion> targetVersion;
	OneSixInstance *m_inst = nullptr;
};
