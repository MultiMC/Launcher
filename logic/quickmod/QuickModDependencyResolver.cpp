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

#include "QuickModDependencyResolver.h"

#include <QWidget>

#include "gui/dialogs/VersionSelectDialog.h"
#include "QuickModVersion.h"
#include "QuickMod.h"
#include "QuickModsList.h"
#include "logic/OneSixInstance.h"
#include "MultiMC.h"
#include "modutils.h"

QuickModDependencyResolver::QuickModDependencyResolver(InstancePtr instance, QWidget *parent)
	: QuickModDependencyResolver(instance, parent, parent)
{
}

QuickModDependencyResolver::QuickModDependencyResolver(InstancePtr instance,
													   QWidget *widgetParent, QObject *parent)
	: QObject(parent), m_widgetParent(widgetParent), m_instance(instance)
{
}

QList<QuickModVersionPtr> QuickModDependencyResolver::resolve(const QList<QuickModUid> &mods)
{
	for (QuickModUid mod : mods)
	{
		bool ok;
		resolve(getVersion(mod, QString(), &ok));
		if (!ok)
		{
			emit error(tr("Didn't select a version for %1").arg(mod.mod()->name()));
			return QList<QuickModVersionPtr>();
		}
	}
	return m_mods.values();
}

QuickModVersionPtr QuickModDependencyResolver::getVersion(const QuickModUid &modUid,
														  const QString &filter, bool *ok)
{
	// FIXME: This only works with 1.6
	const QString predefinedVersion =
		std::dynamic_pointer_cast<OneSixInstance>(m_instance)->getFullVersion()->quickmods
			[modUid];
	VersionSelectDialog dialog(new QuickModVersionList(modUid, m_instance, this),
							   tr("Choose QuickMod version for %1").arg(modUid.mod()->name()),
							   m_widgetParent);
	dialog.setExactFilter(BaseVersionList::NameColumn, filter);
	dialog.setUseLatest(MMC->settings()->get("QuickModAlwaysLatestVersion").toBool());
	// TODO currently, if the version isn't existing anymore it will be updated
	if (!predefinedVersion.isEmpty() &&
		MMC->quickmodslist()->modVersion(modUid, predefinedVersion))
	{
		dialog.setExactFilter(BaseVersionList::NameColumn, predefinedVersion);
	}
	if (dialog.exec() == QDialog::Rejected)
	{
		*ok = false;
		return QuickModVersionPtr();
	}
	*ok = true;
	return std::dynamic_pointer_cast<QuickModVersion>(dialog.selectedVersion());
}

void QuickModDependencyResolver::resolve(const QuickModVersionPtr version)
{
	if (!version)
	{
		return;
	}
	if (m_mods.contains(version->mod.get()) &&
		Util::Version(version->name()) <= Util::Version(m_mods[version->mod.get()]->name()))
	{
		return;
	}
	m_mods.insert(version->mod.get(), version);
	for (auto it = version->dependencies.begin(); it != version->dependencies.end(); ++it)
	{
		if (MMC->quickmodslist()->mods(it.key()).isEmpty())
		{
			emit warning(tr("The dependency from %1 (%2) to %3 cannot be resolved")
							 .arg(version->mod->name(), version->name(), it.key().toString()));
			continue;
		}
		bool ok;
		QuickModVersionPtr dep = getVersion(it.key(), it.value(), &ok);
		if (!ok)
		{
			emit warning(tr("Didn't select a version while resolving from %1 (%2) to %3")
							 .arg(version->mod->name(), version->name(), it.key().toString()));
		}
		if (dep)
		{
			emit success(tr("Successfully resolved dependency from %1 (%2) to %3 (%4)").arg(
				version->mod->name(), version->name(), dep->mod->name(), dep->name()));
			if (m_mods.contains(version->mod.get()) &&
				Util::Version(dep->name()) > Util::Version(m_mods[version->mod.get()]->name()))
			{
				resolve(dep);
			}
			else
			{
				resolve(dep);
			}
		}
		else
		{
			emit warning(tr("The dependency from %1 (%2) to %3 (%4) cannot be resolved").arg(
				version->mod->name(), version->name(), it.key().toString(), it.value()));
		}
	}
}
