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
#include "logic/quickmod/QuickModMetadata.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModsList.h"
#include "logic/quickmod/QuickModSettings.h"
#include "logic/quickmod/QuickModDependencyResolver.h"
#include "logic/OneSixInstance.h"
#include "MultiMC.h"

QuickModDownloadTask::QuickModDownloadTask(std::shared_ptr<OneSixInstance> instance,
										   Bindable *parent)
	: Task(parent), m_instance(instance)
{
}

void QuickModDownloadTask::executeTask()
{
	const bool hasResolveError = QuickModDependencyResolver(m_instance).hasResolveError();

	const QMap<QuickModRef, QPair<QuickModVersionRef, bool>> quickmods =
		m_instance->getFullVersion()->quickmods;
	QList<QuickModRef> mods;
	for (auto it = quickmods.cbegin(); it != quickmods.cend(); ++it)
	{
		const QuickModVersionRef version = it.value().first;
		QuickModMetadataPtr mod = version.isValid() ? MMC->quickmodslist()->mod(version.mod())
											: MMC->quickmodslist()->mod(it.key());
		QuickModVersionPtr ptr = MMC->quickmodslist()->version(version);
		if (!ptr || !MMC->quickmodSettings()->isModMarkedAsExists(mod, version) || hasResolveError ||
				(ptr->needsDeploy() && !MMC->quickmodSettings()->isModMarkedAsInstalled(mod->uid(), version, m_instance)))
		{
			mods.append(mod->uid());
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
		QMap<QuickModRef, QPair<QuickModVersionRef, bool>> mods;
		for (const auto version : installedVersions)
		{
			const auto uid = version->mod->uid();
			bool isManualInstall = quickmods.contains(uid) && quickmods[uid].second;
			mods.insert(version->mod->uid(), qMakePair(version->version(), isManualInstall));
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
