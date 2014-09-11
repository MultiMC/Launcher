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

#include "logic/quickmod/QuickModMetadata.h"
#include "logic/quickmod/QuickModsList.h"
#include "logic/quickmod/QuickModDownloadAction.h"
#include "MultiMC.h"

QuickModDependencyDownloadTask::QuickModDependencyDownloadTask(QList<QuickModRef> mods,
															   QObject *parent)
	: Task(parent), m_mods(mods)
{
	m_netjob.reset(new NetJob("QuickMod Download"));
}

void QuickModDependencyDownloadTask::executeTask()
{
	m_pendingMods.clear();
	m_lastSetPercentage = 0;

	setStatus(tr("Fetching QuickMods files..."));

	for (const QuickModRef mod : m_mods)
	{
		auto mods = MMC->quickmodslist()->mods(mod);
		if (mods.isEmpty()) // means we don't have it yet
		{
			if (mod.updateUrl().isValid()) // do we have the required information to fetch it?
			{
				fetch(mod);
			}
			else
			{
				// TODO fetch info from somewhere?
				if (!wait<bool>("QuickMods.ModMissing", mod.toString()))
				{
					emitFailed(tr("Missing %1").arg(mod.userFacing()));
					return;
				}
			}
		}
		else
		{
			for (const auto m : mods)
			{
				requestDependenciesOf(m);
			}
		}
	}
	if (m_pendingMods.isEmpty())
	{
		emitSucceeded();
	}
	updateProgress();

	m_netjob->start();
}

void QuickModDependencyDownloadTask::modAdded(QuickModMetadataPtr mod)
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
		finish();
	}
}

void QuickModDependencyDownloadTask::finish()
{
	if (m_actions.isEmpty())
	{
		emitSucceeded();
		return;
	}
	QList<QuickModMetadataPtr> addedMods;
	for (auto action : m_actions)
	{
		addedMods.append(action->m_resultMetadata);
	}
	if (wait<bool>("QuickMods.VerifyMods", addedMods))
	{
		for (auto action : m_actions)
		{
			action->add();
		}
		emitSucceeded();
	}
	else
	{
		emitFailed(tr("Didn't verify files"));
	}
}

void QuickModDependencyDownloadTask::fetch(const QuickModRef &mod)
{
	if (!m_requestedMods.contains(mod))
	{
		m_pendingMods.append(mod);
		m_requestedMods.append(mod);

		auto action = std::make_shared<QuickModDownloadAction>(mod.updateUrl(), mod.toString());
		connect(action.get(), &QuickModDownloadAction::succeeded, [this, action](int){modAdded(action->m_resultMetadata);});
		m_actions.append(action);
		m_netjob->addNetAction(action);
	}
}

void QuickModDependencyDownloadTask::requestDependenciesOf(const QuickModMetadataPtr mod)
{
	auto references = mod->references();
	for (auto it = references.begin(); it != references.end(); ++it)
	{
		const QuickModRef modUid = QuickModRef(it.key().toString(), it.value());
		if (!MMC->quickmodslist()->mods(modUid).isEmpty())
		{
			return;
		}
		fetch(modUid);
	}
}
