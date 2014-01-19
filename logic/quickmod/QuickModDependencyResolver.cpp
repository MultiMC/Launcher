#include "QuickModDependencyResolver.h"

#include <QWidget>

#include "gui/dialogs/VersionSelectDialog.h"
#include "QuickModVersion.h"
#include "QuickMod.h"
#include "QuickModsList.h"
#include "MultiMC.h"

QuickModDependencyResolver::QuickModDependencyResolver(BaseInstance *instance, QWidget *parent)
	: QuickModDependencyResolver(instance, parent, parent)
{

}

QuickModDependencyResolver::QuickModDependencyResolver(BaseInstance *instance, QWidget *widgetParent, QObject *parent)
	: QObject(parent), m_widgetParent(widgetParent), m_instance(instance)
{

}

QList<QuickModVersionPtr> QuickModDependencyResolver::resolve(const QList<QuickMod *> &mods)
{
	QList<QuickModVersionPtr> out;
	foreach (QuickMod *mod, mods)
	{
		bool ok;
		out.append(resolve(getVersion(mod, QString(), &ok), out));
		if (!ok)
		{
			emit error(tr("Didn't select a version for %1").arg(mod->name()));
			return QList<QuickModVersionPtr>();
		}
	}
	return out;
}

QuickModVersionPtr QuickModDependencyResolver::getVersion(QuickMod *mod, const QString &filter, bool *ok)
{
	VersionSelectDialog dialog(new QuickModVersionList(mod, m_instance, this),
							   tr("Choose QuickMod version for %1").arg(mod->name()), m_widgetParent);
	dialog.setFilter(BaseVersionList::NameColumn, filter);
	if (dialog.exec() == QDialog::Rejected)
	{
		*ok = false;
		return QuickModVersionPtr();
	}
	*ok = true;
	return std::dynamic_pointer_cast<QuickModVersion>(dialog.selectedVersion());
}

QList<QuickModVersionPtr> QuickModDependencyResolver::resolve(const QuickModVersionPtr version, const QList<QuickModVersionPtr> &exclude)
{
	QList<QuickModVersionPtr> out;
	if (!version)
	{
		emit error(tr("Unknown error"));
		return out;
	}
	if (exclude.contains(version))
	{
		return out;
	}
	out.append(version);
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
			if (!exclude.contains(dep))
			{
				out.append(resolve(dep, out + exclude));
			}
		}
		else
		{
			emit warning(tr("The dependency from %1 (%2) to %3 (%4) cannot be resolved")
					.arg(version->mod->name(), version->name(), it.key(), it.value()));
		}
	}
	return out;
}
