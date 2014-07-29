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

#include "QuickModDownloadTask.h"

#include "logic/net/ByteArrayDownload.h"
#include "logic/quickmod/QuickMod.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModsList.h"
#include "logic/OneSixInstance.h"
#include "MultiMC.h"

QuickModDownloadTask::QuickModDownloadTask(std::shared_ptr<OneSixInstance> instance,
										   Bindable *parent)
	: Task(parent), m_instance(instance)
{
}

void QuickModDownloadTask::executeTask()
{
	const QMap<QuickModUid, QPair<QString, bool>> quickmods =
		std::dynamic_pointer_cast<OneSixInstance>(m_instance)->getFullVersion()->quickmods;
	auto list = MMC->quickmodslist();
	QList<QuickModUid> mods;
	for (auto it = quickmods.cbegin(); it != quickmods.cend(); ++it)
	{
		const QString version = it.value().first;
		QuickModPtr mod = version.isEmpty() ? list->mods(it.key()).first()
											: list->modVersion(it.key(), version)->mod;
		if (mod == 0)
		{
			// TODO fetch info from somewhere?
			if (wait<bool>("QuickMods.ModMissing", it.key().toString()))
			{
				emitFailed(tr("Missing %1").arg(it.key().toString()));
				return;
			}
			else
			{
				continue;
			}
		}
		else
		{
			if (version.isEmpty() || !list->isModMarkedAsExists(mod, version))
			{
				mods.append(mod->uid());
			}
		}
	}

	if (mods.isEmpty())
	{
		emitSucceeded();
		return;
	}

	bool ok = false;
	QList<QuickModVersionPtr> installedVersions =
		wait<QList<QuickModVersionPtr>>("QuickMods.InstallMods", m_instance, mods, &ok);
	if (ok)
	{
		const auto quickmods = m_instance->getFullVersion()->quickmods;
		QMap<QuickModUid, QPair<QString, bool>> mods;
		for (const auto version : installedVersions)
		{
			const auto uid = version->mod->uid();
			bool isManualInstall = quickmods.contains(uid) &&
					quickmods[uid].second;
			mods.insert(version->mod->uid(), qMakePair(version->name(), isManualInstall));
		}
		try
		{
			m_instance->setQuickModVersions(mods);
		}
		catch (...)
		{
			QLOG_ERROR() << "This means that stuff will be downloaded on every instance launch";
		}

		emitSucceeded();
	}
	else
	{
		emitFailed(tr("Failure downloading QuickMods"));
	}
}
