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

#pragma once

#include <QObject>
#include <memory>

class InstanceList;
class QuickModsList;
class BaseInstance;
class OneSixInstance;
class QuickMod;

class QuickModUpdateMonitor : public QObject
{
	Q_OBJECT
public:
	QuickModUpdateMonitor(std::shared_ptr<InstanceList> instances,
						  std::shared_ptr<QuickModsList> quickmods, QObject *parent = 0);

private
slots:
	void instanceListRowsInserted(const QModelIndex &parent, const int start, const int end);
	void instanceListRowsRemoved(const QModelIndex &parent, const int start, const int end);
	void instanceListReset();
	void quickmodsListRowsInserted(const QModelIndex &parent, const int start, const int end);
	void quickmodsListRowsRemoved(const QModelIndex &parent, const int start, const int end);
	void quickmodsListReset();

	void quickmodUpdated();
	void instanceReloaded();

private:
	std::shared_ptr<InstanceList> m_instanceList;
	std::shared_ptr<QuickModsList> m_quickmodsList;

	void checkForInstance(std::shared_ptr<OneSixInstance> instance);
};
