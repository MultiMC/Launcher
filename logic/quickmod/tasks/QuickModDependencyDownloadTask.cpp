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

#include "QuickModDependencyDownloadTask.h"

#include "logic/quickmod/QuickMod.h"
#include "logic/quickmod/QuickModsList.h"
#include "MultiMC.h"

QuickModDependencyDownloadTask::QuickModDependencyDownloadTask(QList<QuickModUid> mods,
															   QObject *parent)
	: Task(parent), m_mods(mods)
{
}

void QuickModDependencyDownloadTask::executeTask()
{
	m_pendingMods.clear();
	m_lastSetPercentage = 0;

	setStatus(tr("Fetching QuickMods files..."));

	connect(MMC->quickmodslist().get(), &QuickModsList::modAdded, this,
			&QuickModDependencyDownloadTask::modAdded);
	// TODO we cannot know if this is about us
	connect(MMC->quickmodslist().get(), &QuickModsList::error, this,
			&QuickModDependencyDownloadTask::emitFailed);

	for (const QuickModUid mod : m_mods)
	{
		for (auto variant : mod.mods())
		{
			requestDependenciesOf(variant);
		}
	}
	if (m_pendingMods.isEmpty())
	{
		emitSucceeded();
	}
	updateProgress();
}

void QuickModDependencyDownloadTask::modAdded(QuickModPtr mod)
{
	if (m_pendingMods.contains(mod->uid()))
	{
		m_pendingMods.removeAll(mod->uid());
		m_mods.append(mod->uid());
		requestDependenciesOf(mod);
	}
	updateProgress();
}

void QuickModDependencyDownloadTask::updateProgress()
{
	int max = m_requestedMods.size();
	int current = max - m_pendingMods.size();
	double percentage = (double)current * 100.0f / (double)max;
	m_lastSetPercentage = qMax(m_lastSetPercentage, qCeil(percentage));
	setProgress(m_lastSetPercentage);
	if (m_pendingMods.isEmpty())
	{
		emitSucceeded();
	}
}

void QuickModDependencyDownloadTask::requestDependenciesOf(const QuickModPtr mod)
{
	auto references = mod->references();
	for (auto it = references.begin(); it != references.end(); ++it)
	{
		const QuickModUid modUid = it.key();
		if (!MMC->quickmodslist()->mods(modUid).isEmpty())
		{
			return;
		}
		if (!m_requestedMods.contains(modUid))
		{
			MMC->quickmodslist()->registerMod(it.value());
			m_pendingMods.append(modUid);
			m_requestedMods.append(modUid);
		}
	}
}
