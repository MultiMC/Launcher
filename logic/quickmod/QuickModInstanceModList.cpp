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

#include "QuickModInstanceModList.h"

#include "MultiMC.h"
#include "logic/quickmod/QuickModsList.h"
#include "logic/quickmod/QuickModLibraryInstaller.h"
#include "logic/quickmod/QuickModDependencyResolver.h"
#include "modutils.h"

QuickModInstanceModList::QuickModInstanceModList(const Type type,
												 std::shared_ptr<OneSixInstance> instance,
												 std::shared_ptr<ModList> modList,
												 QObject *parent)
	: QAbstractListModel(parent), m_instance(instance), m_modList(modList), m_type(type)
{
	connect(m_modList.get(), &ModList::modelAboutToBeReset, this,
			&QuickModInstanceModList::beginResetModel);
	connect(m_modList.get(), &ModList::modelReset, this,
			&QuickModInstanceModList::endResetModel);
	connect(
		m_modList.get(), &ModList::rowsAboutToBeInserted,
		[this](const QModelIndex &parent, const int first, const int last)
		{ beginInsertRows(parent, first + quickmods().size(), last + quickmods().size()); });
	connect(m_modList.get(), &ModList::rowsInserted, this,
			&QuickModInstanceModList::endInsertRows);
	connect(m_modList.get(), &ModList::dataChanged,
			[this](const QModelIndex &tl, const QModelIndex &br, const QVector<int> &roles)
			{ emit dataChanged(mapFromModList(tl), mapFromModList(br), roles); });
	connect(m_instance.get(), &OneSixInstance::versionReloaded, this,
			&QuickModInstanceModList::resetModel);

	connect(MMC->quickmodslist().get(), &QuickModsList::rowsInserted,
			[this](const QModelIndex &parent, const int first, const int last)
			{
		for (int i = first; i < (last + 1); ++i)
		{
			auto mod = MMC->quickmodslist()->modAt(i);
			connect(mod.get(), &QuickMod::iconUpdated, this,
					&QuickModInstanceModList::quickmodIconUpdated);
		}
	});
	connect(MMC->quickmodslist().get(), &QuickModsList::rowsAboutToBeRemoved,
			[this](const QModelIndex &parent, const int first, const int last)
			{
		for (int i = first; i < (last + 1); ++i)
		{
			disconnect(MMC->quickmodslist()->modAt(i).get(), 0, this, 0);
		}
	});
}

QuickModInstanceModList::~QuickModInstanceModList()
{
	disconnect(this, 0, 0, 0);
}

int QuickModInstanceModList::rowCount(const QModelIndex &parent) const
{
	return quickmods().size() + m_modList->rowCount(parent);
}
int QuickModInstanceModList::columnCount(const QModelIndex &parent) const
{
	return 3;
}

QVariant QuickModInstanceModList::data(const QModelIndex &index, int role) const
{
	if (isModListArea(index))
	{
		return m_modList->data(mapToModList(index), role);
	}

	const int row = index.row();
	const int col = index.column();

	QuickModPtr mod = modAt(row);
	if (!mod)
	{
		return QVariant();
	}

	const QuickModVersionID versionName = quickmods()[mod->uid()];
	QuickModVersionPtr version = versionName.findVersion();

	switch (role)
	{
	case Qt::DecorationRole:
		switch (col)
		{
		case NameColumn:
			return mod->icon();
		case VersionColumn:
			if (mod->latestVersion(m_instance->intendedVersionId()) && versionName.isValid())
			{
				const QuickModVersionID latest =
					mod->latestVersion(m_instance->intendedVersionId())->version();
				if (latest > versionName) // there's a new version
				{
					return QPixmap(":/icons/badges/updateavailable.png");
				}
				else if (!version) // current version is not available anymore
				{
					return QPixmap(":/icons/badges/updateavailable.png");
				}
			}
		default:
			return QVariant();
		}
	case Qt::DisplayRole:
		switch (index.column())
		{
		case NameColumn:
			return mod->name();
		case VersionColumn:
			if (version)
			{
				return version->name();
			}
			return tr("TBD");
		default:
			return QVariant();
		}
	case Qt::ToolTipRole:
		return mod->internalUid();
	case Qt::FontRole:
		if (col == VersionColumn)
		{
			if (!versionName.isValid())
			{
				QFont font = qApp->font();
				font.setItalic(true);
				return font;
			}
		}
	default:
		return QVariant();
	}
}
QVariant QuickModInstanceModList::headerData(int section, Qt::Orientation orientation, int role)
	const
{
	return m_modList->headerData(section, orientation, role);
}
Qt::ItemFlags QuickModInstanceModList::flags(const QModelIndex &index) const
{
	if (isModListArea(index))
	{
		return m_modList->flags(mapToModList(index));
	}
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool QuickModInstanceModList::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (isModListArea(index))
	{
		return m_modList->setData(mapToModList(index), value, role);
	}
	return false;
}
QMimeData *QuickModInstanceModList::mimeData(const QModelIndexList &indexes) const
{
	QModelIndexList i;
	for (auto index : indexes)
	{
		if (isModListArea(index))
		{
			i.append(mapToModList(index));
		}
	}
	return m_modList->mimeData(i);
}
QStringList QuickModInstanceModList::mimeTypes() const
{
	return m_modList->mimeTypes();
}
bool QuickModInstanceModList::dropMimeData(const QMimeData *data, Qt::DropAction action,
										   int row, int column, const QModelIndex &parent)
{
	if (isModListArea(index(row, column, parent)))
	{
		return m_modList->dropMimeData(data, action, row - quickmods().size(), column, parent);
	}
	return QAbstractListModel::dropMimeData(data, action, row, column, parent);
}
Qt::DropActions QuickModInstanceModList::supportedDragActions() const
{
	return m_modList->supportedDragActions();
}
Qt::DropActions QuickModInstanceModList::supportedDropActions() const
{
	return m_modList->supportedDropActions();
}

void QuickModInstanceModList::quickmodIconUpdated()
{
	emit dataChanged(index(0, NameColumn), index(rowCount(), NameColumn),
					 QVector<int>() << Qt::DecorationRole);
}

QMap<QuickModUid, QuickModVersionID> QuickModInstanceModList::quickmods() const
{
	if (m_type == ResourcePacks || m_type == CoreMods || m_type == TexturePacks)
	{
		return QMap<QuickModUid, QuickModVersionID>();
	}
	QMap<QuickModUid, QuickModVersionID> out;
	auto mods = m_instance->getFullVersion()->quickmods;
	for (auto it = mods.begin(); it != mods.end(); ++it)
	{
		out.insert(QuickModUid(it.key()), it.value().first);
	}
	return out;
}

QuickModPtr QuickModInstanceModList::modAt(const int row) const
{
	auto uid = quickmods().keys()[row];
	if (MMC->quickmodslist()->mods(uid).isEmpty())
	{
		return 0;
	}
	if (!quickmods()[uid].isValid())
	{
		return MMC->quickmodslist()->mods(uid).first();
	}
	return quickmods()[uid].findMod();
}

QModelIndex QuickModInstanceModList::mapToModList(const QModelIndex &index) const
{
	return m_modList->index(index.row() - quickmods().size(), index.column(), QModelIndex());
}
QModelIndex QuickModInstanceModList::mapFromModList(const QModelIndex &index) const
{
	return this->index(index.row() + quickmods().size(), index.column(), QModelIndex());
}
bool QuickModInstanceModList::isModListArea(const QModelIndex &index) const
{
	return index.row() >= quickmods().size();
}

QList<QuickModUid> QuickModInstanceModList::findOrphans() const
{
	return QuickModDependencyResolver(m_instance).resolveOrphans();
}

void QuickModInstanceModList::updateMods(const QModelIndexList &list)
{
	QMap<QuickModUid, QPair<QuickModVersionID, bool>> mods;
	for (const auto index : list)
	{
		if (isModListArea(index))
		{
			continue;
		}
		const auto uid = modAt(index.row())->uid();
		mods.insert(uid,
					qMakePair(QuickModVersionID(), m_instance->getFullVersion()->quickmods[uid].second));
	}
	if (!mods.isEmpty())
	{
		m_instance->setQuickModVersions(mods);
	}
}
void QuickModInstanceModList::removeMods(const QModelIndexList &list)
{
	QList<QuickModUid> mods;
	for (const auto index : list)
	{
		if (isModListArea(index))
		{
			continue;
		}
		const auto mod = modAt(index.row());
		if (auto version = quickmods()[mod->uid()].findVersion())
		{
			QuickModLibraryInstaller(version).remove(m_instance.get());
		}
		mods.append(mod->uid());
	}
	const auto resolved = QuickModDependencyResolver(m_instance).resolveChildren(mods);
	if (mods != resolved)
	{
		if (!wait<bool>("QuickMods.ConfirmRemoval", resolved))
		{
			return;
		}
		mods = resolved;
	}
	if (!mods.isEmpty())
	{
		m_instance->removeQuickMods(mods);
	}
}

QuickModInstanceModListProxy::QuickModInstanceModListProxy(QuickModInstanceModList *list,
														   QObject *parent)
	: QSortFilterProxyModel(parent), m_list(list)
{
	setSourceModel(m_list);
}

bool QuickModInstanceModListProxy::filterAcceptsRow(int source_row,
													const QModelIndex &source_parent) const
{
	const QModelIndex index = m_list->index(source_row, 0, source_parent);
	if (m_list->isModListArea(index))
	{
		const QString file = index.data(ModList::ModFileRole).toString();
		auto quickmods = m_list->quickmods();
		for (auto it = quickmods.begin(); it != quickmods.end(); ++it)
		{
			const QuickModUid uid = it.key();
			if (!uid.isValid() || MMC->quickmodslist()->mods(uid).isEmpty())
			{
				continue;
			}
			auto mod = MMC->quickmodslist()->mods(uid).first();
			if (!mod)
			{
				continue;
			}
			const QString existingFile = MMC->quickmodslist()->installedModFiles(
				mod->uid(), m_list->m_instance.get())[it.value()];
			if (file == existingFile)
			{
				return false;
			}
		}
	}
	return true;
}
