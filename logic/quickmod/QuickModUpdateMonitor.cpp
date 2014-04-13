#include "QuickModUpdateMonitor.h"

#include "QuickModsList.h"
#include "QuickMod.h"
#include "logic/lists/InstanceList.h"
#include "logic/OneSixInstance.h"

QuickModUpdateMonitor::QuickModUpdateMonitor(std::shared_ptr<InstanceList> instances, std::shared_ptr<QuickModsList> quickmods, QObject *parent)
	: QObject(parent), m_instanceList(instances), m_quickmodsList(quickmods)
{
	connect(m_instanceList.get(), &InstanceList::rowsInserted, this, &QuickModUpdateMonitor::instanceListRowsInserted);
	connect(m_instanceList.get(), &InstanceList::rowsAboutToBeRemoved, this, &QuickModUpdateMonitor::instanceListRowsRemoved);
	connect(m_instanceList.get(), &InstanceList::modelAboutToBeReset, this, &QuickModUpdateMonitor::instanceListReset);
	connect(m_quickmodsList.get(), &QuickModsList::rowsInserted, this, &QuickModUpdateMonitor::quickmodsListRowsInserted);
	connect(m_quickmodsList.get(), &QuickModsList::rowsAboutToBeRemoved, this, &QuickModUpdateMonitor::quickmodsListRowsRemoved);
	connect(m_quickmodsList.get(), &QuickModsList::modelAboutToBeReset, this, &QuickModUpdateMonitor::quickmodsListReset);
}

void QuickModUpdateMonitor::instanceListRowsInserted(const QModelIndex &parent, const int start, const int end)
{
	qDebug() << "###########################################" << start << end;
	for (int i = start; i < end; ++i)
	{
		auto instance = std::dynamic_pointer_cast<OneSixInstance>(m_instanceList->at(i));
		connect(instance.get(), &OneSixInstance::versionReloaded, this, &QuickModUpdateMonitor::instanceReloaded);
		checkForInstance(instance);
	}
}
void QuickModUpdateMonitor::instanceListRowsRemoved(const QModelIndex &parent, const int start, const int end)
{
	for (int i = start; i < end; ++i)
	{
		auto instance = m_instanceList->at(i);
		disconnect(instance.get(), 0, this, 0);
	}
}
void QuickModUpdateMonitor::instanceListReset()
{
	for (int i = 0; i< m_instanceList->count(); ++i)
	{
		disconnect(m_instanceList->at(i).get(), 0, this, 0);
	}
}
void QuickModUpdateMonitor::quickmodsListRowsInserted(const QModelIndex &parent, const int start, const int end)
{
	for (int i = start; i < (end + 1); ++i)
	{
		auto mod = m_quickmodsList->modAt(i);
		connect(mod, &QuickMod::versionsUpdated, this, &QuickModUpdateMonitor::quickmodUpdated);
	}
}
void QuickModUpdateMonitor::quickmodsListRowsRemoved(const QModelIndex &parent, const int start, const int end)
{
	for (int i = start; i < end; ++i)
	{
		auto mod = m_quickmodsList->modAt(i);
		disconnect(mod, 0, this, 0);
	}
}
void QuickModUpdateMonitor::quickmodsListReset()
{
	for (int i = 0; i < m_quickmodsList->numMods(); ++i)
	{
		disconnect(m_quickmodsList->modAt(i), 0, this, 0);
	}
}

void QuickModUpdateMonitor::quickmodUpdated()
{
	for (int i = 0; i < m_instanceList->count(); ++i)
	{
		checkForInstance(std::dynamic_pointer_cast<OneSixInstance>(m_instanceList->at(i)));
	}
}
void QuickModUpdateMonitor::instanceReloaded()
{
	checkForInstance(std::shared_ptr<OneSixInstance>(qobject_cast<OneSixInstance *>(sender())));
}

void QuickModUpdateMonitor::checkForInstance(std::shared_ptr<OneSixInstance> instance)
{
	if (!m_quickmodsList->updatedModsForInstance(instance).isEmpty())
	{
		instance->setFlag(BaseInstance::UpdateAvailable);
	}
}
