#include "QuickModVersionList.h"
#include "QuickModVersionRef.h"
#include "QuickModVersion.h"
#include "logger/QsLog.h"

QuickModVersionList::QuickModVersionList(QuickModRef mod, InstancePtr instance, QObject *parent)
	: BaseVersionList(parent), m_mod(mod), m_instance(instance)
{
	for (auto version : QList<QuickModVersionRef>())// FIXME m_mod.findVersions())
	{
		if (version.findVersion()->compatibleVersions.contains(m_instance->intendedVersionId()))
		{
			m_versions.append(version);
		}
	}
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
	if(i < m_versions.size() && i >= 0)
		return m_versions[i].findVersion();

	QLOG_WARN() << "QuickModVersionList::at -> Index out of range (" << i << ")";
	return QuickModVersionPtr();
}

int QuickModVersionList::count() const
{
	return m_versions.count();
}
