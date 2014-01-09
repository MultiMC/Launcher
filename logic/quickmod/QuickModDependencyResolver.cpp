#include "QuickModDependencyResolver.h"

#include <QWidget>

#include "gui/dialogs/VersionSelectDialog.h"
#include "QuickModVersion.h"

QuickModDependencyResolver::QuickModDependencyResolver(BaseInstance *instance, QWidget *parent)
	: QuickModDependencyResolver(instance, parent, parent)
{

}

QuickModDependencyResolver::QuickModDependencyResolver(BaseInstance *instance, QWidget *widgetParent, QObject *parent)
	: QObject(parent), m_widgetParent(widgetParent), m_instance(instance)
{

}

// TODO do actual resolving
QList<QuickModVersionPtr> QuickModDependencyResolver::resolve(const QList<QuickMod *> &mods)
{
	QList<QuickModVersionPtr> out;
	foreach (QuickMod *mod, mods)
	{
		VersionSelectDialog dialog(new QuickModVersionList(mod, m_instance, this),
								   tr("Choose QuickMod version"), m_widgetParent);
		dialog.setFilter(BaseVersionList::NameColumn, QString());
		if (dialog.exec() == QDialog::Rejected)
		{
			continue;
		}
		BaseVersionPtr versionPtr = dialog.selectedVersion();
		QuickModVersionPtr quickmodVersionPtr =
			std::dynamic_pointer_cast<QuickModVersion>(versionPtr);
		out.append(quickmodVersionPtr);
	}
	return out;
}
