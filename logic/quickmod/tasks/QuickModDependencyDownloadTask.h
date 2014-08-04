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

#include "logic/tasks/Task.h"
#include "logic/quickmod/QuickMod.h"

class QuickModDependencyDownloadTask : public Task
{
	Q_OBJECT
public:
	explicit QuickModDependencyDownloadTask(QList<QuickModRef> mods, QObject *parent = 0);

protected:
	void executeTask();

private
slots:
	void modAdded(QuickModPtr mod);

private:
	QList<QuickModRef> m_mods;
	// list of mods we are still waiting for
	QList<QuickModRef> m_pendingMods;
	// list of mods we have requested
	QList<QuickModRef> m_requestedMods;

	// list of mods we have received that are in the sandbox
	QList<QuickModPtr> m_sandboxedMods;

	int m_lastSetPercentage;
	void updateProgress();
	void finish();

	void requestDependenciesOf(const QuickModPtr mod);
};
