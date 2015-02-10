/* Copyright 2013-2015 MultiMC Contributors
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

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <pathutils.h>

#include "minecraft/MinecraftProfile.h"
#include "ProfileUtils.h"
#include "NullProfileStrategy.h"
#include "onesix/OneSixFormat.h"
#include "Resources.h"

MinecraftProfile::MinecraftProfile(ProfileStrategy *strategy)
	: QAbstractListModel()
{
	setStrategy(strategy);
	clear();
}

void MinecraftProfile::setStrategy(ProfileStrategy* strategy)
{
	Q_ASSERT(strategy != nullptr);

	if(m_strategy != nullptr)
	{
		delete m_strategy;
		m_strategy = nullptr;
	}
	m_strategy = strategy;
	m_strategy->profile = this;
}

ProfileStrategy* MinecraftProfile::strategy()
{
	return m_strategy;
}

void MinecraftProfile::reload()
{
	beginResetModel();
	m_strategy->load();
	reapply();
	endResetModel();
}

void MinecraftProfile::clear()
{
	resources.clear();
}

void MinecraftProfile::clearPatches()
{
	beginResetModel();
	VersionPatches.clear();
	endResetModel();
}

void MinecraftProfile::appendPatch(PackagePtr patch)
{
	int index = VersionPatches.size();
	beginInsertRows(QModelIndex(), index, index);
	VersionPatches.append(patch);
	endInsertRows();
}

bool MinecraftProfile::canRemove(const int index) const
{
	return VersionPatches.at(index)->isMoveable();
}

bool MinecraftProfile::remove(const int index)
{
	if (!canRemove(index))
	{
		qDebug() << "Patch" << index << "is non-removable";
		return false;
	}

	if(!m_strategy->removePatch(VersionPatches.at(index)))
	{
		qCritical() << "Patch" << index << "could not be removed";
		return false;
	}

	beginRemoveRows(QModelIndex(), index, index);
	VersionPatches.removeAt(index);
	endRemoveRows();
	reapply();
	saveCurrentOrder();
	return true;
}

bool MinecraftProfile::remove(const QString id)
{
	int i = 0;
	for (auto patch : VersionPatches)
	{
		if (patch->getPatchID() == id)
		{
			return remove(i);
		}
		i++;
	}
	return false;
}

QString MinecraftProfile::versionFileId(const int index) const
{
	if (index < 0 || index >= VersionPatches.size())
	{
		return QString();
	}
	return VersionPatches.at(index)->getPatchID();
}

PackagePtr MinecraftProfile::versionPatch(const QString &id)
{
	for (auto file : VersionPatches)
	{
		if (file->getPatchID() == id)
		{
			return file;
		}
	}
	return 0;
}

PackagePtr MinecraftProfile::versionPatch(int index)
{
	if(index < 0 || index >= VersionPatches.size())
		return 0;
	return VersionPatches[index];
}

QVariant MinecraftProfile::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (row < 0 || row >= VersionPatches.size())
		return QVariant();

	if (role == Qt::DisplayRole)
	{
		switch (column)
		{
		case 0:
			return VersionPatches.at(row)->getPatchName();
		case 1:
			return VersionPatches.at(row)->getPatchVersion();
		default:
			return QVariant();
		}
	}
	return QVariant();
}
QVariant MinecraftProfile::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal)
	{
		if (role == Qt::DisplayRole)
		{
			switch (section)
			{
			case 0:
				return tr("Name");
			case 1:
				return tr("Version");
			default:
				return QVariant();
			}
		}
	}
	return QVariant();
}
Qt::ItemFlags MinecraftProfile::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

int MinecraftProfile::rowCount(const QModelIndex &parent) const
{
	return VersionPatches.size();
}

int MinecraftProfile::columnCount(const QModelIndex &parent) const
{
	return 2;
}

void MinecraftProfile::saveCurrentOrder() const
{
	ProfileUtils::PatchOrder order;
	for(auto item: VersionPatches)
	{
		if(!item->isMoveable())
			continue;
		order.append(item->getPatchID());
	}
	m_strategy->saveOrder(order);
}

void MinecraftProfile::move(const int index, const MoveDirection direction)
{
	int theirIndex;
	if (direction == MoveUp)
	{
		theirIndex = index - 1;
	}
	else
	{
		theirIndex = index + 1;
	}

	if (index < 0 || index >= VersionPatches.size())
		return;
	if (theirIndex >= rowCount())
		theirIndex = rowCount() - 1;
	if (theirIndex == -1)
		theirIndex = rowCount() - 1;
	if (index == theirIndex)
		return;
	int togap = theirIndex > index ? theirIndex + 1 : theirIndex;

	auto from = versionPatch(index);
	auto to = versionPatch(theirIndex);

	if (!from || !to || !to->isMoveable() || !from->isMoveable())
	{
		return;
	}
	beginMoveRows(QModelIndex(), index, index, QModelIndex(), togap);
	VersionPatches.swap(index, theirIndex);
	endMoveRows();
	saveCurrentOrder();
	reapply();
}

void MinecraftProfile::reapply()
{
	clear();
	for(auto file: VersionPatches)
	{
		file->resources.applyTo(&resources);
	}
	resources.finalize();
}

void MinecraftProfile::installJarMods(QStringList selectedFiles)
{
	m_strategy->installJarMods(selectedFiles);
}
