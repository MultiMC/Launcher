#include "QuickModVersionList.h"
#include "QuickModVersionRef.h"
#include "QuickModVersion.h"
#include "QuickModsList.h"
#include "logger/QsLog.h"
#include "MultiMC.h"

QuickModVersionList::QuickModVersionList(QuickModRef mod, InstancePtr instance, QObject *parent)
	: BaseVersionList(parent), m_mod(mod), m_instance(instance)
{
	m_versions = MMC->quickmodslist()->versions(mod, m_instance->intendedVersionId());
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
	{
		return MMC->quickmodslist()->version(m_versions[i]);
	}

	QLOG_WARN() << "QuickModVersionList::at -> Index out of range (" << i << ")";
	return QuickModVersionPtr();
}

int QuickModVersionList::count() const
{
	return m_versions.count();
}
