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
		out.append(resolve(getVersion(mod), out));
	}
	return out;
}

QuickModVersionPtr QuickModDependencyResolver::getVersion(QuickMod *mod, const QString &filter)
{
	VersionSelectDialog dialog(new QuickModVersionList(mod, m_instance, this),
							   tr("Choose QuickMod version for %1").arg(mod->name()), m_widgetParent);
	dialog.setFilter(BaseVersionList::NameColumn, filter);
	if (dialog.exec() == QDialog::Rejected)
	{
		return QuickModVersionPtr();
	}
	return std::dynamic_pointer_cast<QuickModVersion>(dialog.selectedVersion());
}

QList<QuickModVersionPtr> QuickModDependencyResolver::resolve(const QuickModVersionPtr mod, const QList<QuickModVersionPtr> &exclude)
{
	QList<QuickModVersionPtr> out;
	if (!mod)
	{
		emit totallyUnknownError();
		return out;
	}
	out.append(mod);
	for (auto it = mod->dependencies.begin(); it != mod->dependencies.end(); ++it)
	{
		QuickMod *depMod = MMC->quickmodslist()->mod(it.key());
		if (!depMod)
		{
			emit unresolvedDependency(mod, it.key());
			continue;
		}
		QuickModVersionPtr dep = getVersion(depMod, it.value());
		if (dep)
		{
			emit resolvedDependency(mod, dep);
			if (!exclude.contains(dep))
			{
				out.append(resolve(dep, out + exclude));
			}
		}
		else
		{
			emit didNotSelectVersionError(mod, it.key());
		}
	}
	return out;
}
