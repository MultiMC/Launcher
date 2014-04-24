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

QuickModDependencyResolver::QuickModDependencyResolver(InstancePtr instance, QWidget *widgetParent, QObject *parent)
	: QObject(parent), m_widgetParent(widgetParent), m_instance(instance)
{

}

QList<QuickModVersionPtr> QuickModDependencyResolver::resolve(const QList<QuickMod *> &mods)
{
	foreach (QuickMod *mod, mods)
	{
		bool ok;
		resolve(getVersion(mod, QString(), &ok));
		if (!ok)
		{
			emit error(tr("Didn't select a version for %1").arg(mod->name()));
			return QList<QuickModVersionPtr>();
		}
	}
	return m_mods.values();
}

QuickModVersionPtr QuickModDependencyResolver::getVersion(QuickMod *mod, const QString &filter, bool *ok)
{
	const QString predefinedVersion = std::dynamic_pointer_cast<OneSixInstance>(m_instance)->getFullVersion()->quickmods.value(mod->uid());
	VersionSelectDialog dialog(new QuickModVersionList(mod, m_instance.get(), this),
							   tr("Choose QuickMod version for %1").arg(mod->name()), m_widgetParent);
	dialog.setFilter(BaseVersionList::NameColumn, filter);
	dialog.setUseLatest(true); // TODO: Make a setting
	// TODO currently, if the version isn't existing anymore it will be updated
	if (!predefinedVersion.isEmpty() && mod->version(predefinedVersion))
	{
		dialog.setFilter(BaseVersionList::NameColumn, predefinedVersion);
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
	if (m_mods.contains(version->mod) && Util::Version(version->name()) <= Util::Version(m_mods[version->mod]->name()))
	{
		return;
	}
	m_mods.insert(version->mod, version);
	for (auto it = version->dependencies.begin(); it != version->dependencies.end(); ++it)
	{
		QuickMod *depMod = MMC->quickmodslist()->mod(it.key());
		if (!depMod)
		{
			emit warning(tr("The dependency from %1 (%2) to %3 cannot be resolved")
						 .arg(version->mod->name(), version->name(), it.key()));
			continue;
		}
		bool ok;
		QuickModVersionPtr dep = getVersion(depMod, it.value(), &ok);
		if (!ok)
		{
			emit warning(tr("Didn't select a version while resolving from %1 (%2) to %3")
						 .arg(version->mod->name(), version->name(), it.key()));
		}
		if (dep)
		{
			emit success(tr("Successfully resolved dependency from %1 (%2) to %3 (%4)")
						 .arg(version->mod->name(), version->name(), dep->mod->name(), dep->name()));
			if (m_mods.contains(version->mod) && Util::Version(dep->name()) > Util::Version(m_mods[version->mod]->name()))
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
			emit warning(tr("The dependency from %1 (%2) to %3 (%4) cannot be resolved")
						 .arg(version->mod->name(), version->name(), it.key(), it.value()));
		}
	}
}
