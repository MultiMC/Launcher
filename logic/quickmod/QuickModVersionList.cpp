#include "QuickModVersionList.h"
#include "QuickModVersionRef.h"
#include "QuickModVersion.h"

QuickModVersionList::QuickModVersionList(QuickModRef mod, InstancePtr instance, QObject *parent)
	: BaseVersionList(parent), m_mod(mod), m_instance(instance)
{
}

Task *QuickModVersionList::getLoadTask()
{
	return 0;
}
bool QuickModVersionList::isLoaded()
{
	return true;
}

const BaseVersionPtr QuickModVersionList::at(int i) const
{
	return versions().at(i).findVersion();
}
int QuickModVersionList::count() const
{
	return versions().count();
}

QList<QuickModVersionRef> QuickModVersionList::versions() const
{
	// TODO repository priority
	QList<QuickModVersionRef> out;
	for (auto version : m_mod.findVersions())
	{
		if (version.findVersion()->compatibleVersions.contains(m_instance->intendedVersionId()))
		{
			out.append(version);
		}
	}
	return out;
}
