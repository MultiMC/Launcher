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

#include "QuickModUpdateMonitor.h"

#include "QuickModsList.h"
#include "QuickModMetadata.h"
#include "logic/InstanceList.h"
#include "logic/OneSixInstance.h"

QuickModUpdateMonitor::QuickModUpdateMonitor(std::shared_ptr<InstanceList> instances,
											 std::shared_ptr<QuickModsList> quickmods,
											 QObject *parent)
	: QObject(parent), m_instanceList(instances), m_quickmodsList(quickmods)
{
	connect(m_instanceList.get(), &InstanceList::rowsInserted, this,
			&QuickModUpdateMonitor::instanceListRowsInserted);
	connect(m_instanceList.get(), &InstanceList::rowsAboutToBeRemoved, this,
			&QuickModUpdateMonitor::instanceListRowsRemoved);
	connect(m_instanceList.get(), &InstanceList::modelAboutToBeReset, this,
			&QuickModUpdateMonitor::instanceListAboutToBeReset);
	connect(m_instanceList.get(), &InstanceList::modelReset, this,
			&QuickModUpdateMonitor::instanceListReset);

	connect(m_quickmodsList.get(), &QuickModsList::rowsInserted, this,
			&QuickModUpdateMonitor::quickmodsListRowsInserted);
	connect(m_quickmodsList.get(), &QuickModsList::rowsAboutToBeRemoved, this,
			&QuickModUpdateMonitor::quickmodsListRowsRemoved);
	connect(m_quickmodsList.get(), &QuickModsList::modelAboutToBeReset, this,
			&QuickModUpdateMonitor::quickmodsListAboutToBeReset);
	connect(m_quickmodsList.get(), &QuickModsList::modelReset, this,
			&QuickModUpdateMonitor::quickmodsListReset);
}

void QuickModUpdateMonitor::instanceListRowsInserted(const QModelIndex &parent, const int start,
													 const int end)
{
	for (int i = start; i < (end + 1); ++i)
	{
		auto instance = std::dynamic_pointer_cast<OneSixInstance>(m_instanceList->at(i));
		if (!instance)
		{
			continue;
		}
		m_instances.insert(instance.get(), instance);
		connect(instance.get(), &OneSixInstance::versionReloaded, this,
				&QuickModUpdateMonitor::instanceReloaded);
		checkForInstance(instance);
	}
}

void QuickModUpdateMonitor::instanceListRowsRemoved(const QModelIndex &parent, const int start,
													const int end)
{
	for (int i = start; i < end; ++i)
	{
		auto instance = m_instanceList->at(i);
		m_instances.remove(instance.get());
		disconnect(instance.get(), 0, this, 0);
	}
}

void QuickModUpdateMonitor::instanceListAboutToBeReset()
{
	for (int i = 0; i < m_instanceList->count(); ++i)
	{
		disconnect(m_instanceList->at(i).get(), 0, this, 0);
	}
}

void QuickModUpdateMonitor::instanceListReset()
{
	instanceListRowsInserted(QModelIndex(), 0, m_instanceList->rowCount(QModelIndex()) - 1);
}

void QuickModUpdateMonitor::quickmodsListRowsInserted(const QModelIndex &parent,
													  const int start, const int end)
{
	for (int i = start; i < (end + 1); ++i)
	{
		auto mod = m_quickmodsList->modAt(i);
		// connect to signals here
	}
}

void QuickModUpdateMonitor::quickmodsListRowsRemoved(const QModelIndex &parent, const int start,
													 const int end)
{
	for (int i = start; i < end; ++i)
	{
		auto mod = m_quickmodsList->modAt(i);
		disconnect(mod.get(), 0, this, 0);
	}
}

void QuickModUpdateMonitor::quickmodsListAboutToBeReset()
{
	for (int i = 0; i < m_quickmodsList->numMods(); ++i)
	{
		disconnect(m_quickmodsList->modAt(i).get(), 0, this, 0);
	}
}

void QuickModUpdateMonitor::quickmodsListReset()
{
	quickmodsListRowsInserted(QModelIndex(), 0, m_quickmodsList->rowCount(QModelIndex()) - 1);
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
	checkForInstance(std::dynamic_pointer_cast<OneSixInstance>(m_instances[qobject_cast<BaseInstance *>(sender())]));
}

void QuickModUpdateMonitor::checkForInstance(std::shared_ptr<OneSixInstance> instance)
{
	if (!m_quickmodsList->updatedModsForInstance(instance).isEmpty())
	{
		instance->setFlag(BaseInstance::UpdateAvailable);
	}
	else
	{
		instance->unsetFlag(BaseInstance::UpdateAvailable);
	}
}
