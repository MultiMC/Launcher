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
#include "QuickModsList.h"
#include "QuickModLibraryInstaller.h"
#include "modutils.h"

QuickModInstanceModList::QuickModInstanceModList(const Type type, BaseInstance *instance,
												 std::shared_ptr<ModList> modList,
												 QObject *parent)
	: QAbstractListModel(parent), m_instance(instance), m_modList(modList), m_type(type)
{
	Q_ASSERT(dynamic_cast<OneSixInstance *>(instance));
	connect(m_modList.get(), &ModList::modelAboutToBeReset, this,
			&QuickModInstanceModList::beginResetModel);
	connect(m_modList.get(), &ModList::modelReset, this,
			&QuickModInstanceModList::endResetModel);
	connect(m_modList.get(), &ModList::rowsAboutToBeInserted,
			[this](const QModelIndex &parent, const int first, const int last)
	{ beginInsertRows(parent, first + quickmods().size(), last + quickmods().size()); });
	connect(m_modList.get(), &ModList::rowsInserted, this,
			&QuickModInstanceModList::endInsertRows);
	connect(m_modList.get(), &ModList::dataChanged,
			[this](const QModelIndex &tl, const QModelIndex &br, const QVector<int> &roles)
	{ emit dataChanged(mapFromModList(tl), mapFromModList(br), roles); });
	connect(dynamic_cast<OneSixInstance *>(m_instance),
			&OneSixInstance::versionReloaded, this, &QuickModInstanceModList::resetModel);

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

	const QString versionName = quickmods()[mod->uid()];
	QuickModVersionPtr version = mod->version(versionName);

	switch (role)
	{
	case Qt::DecorationRole:
		switch (col)
		{
		case NameColumn:
			return mod->icon();
		case VersionColumn:
			if (mod->latestVersion(m_instance->intendedVersionId()) && !versionName.isEmpty())
			{
				const QString latest =
					mod->latestVersion(m_instance->intendedVersionId())->name();
				if (Util::Version(latest) > Util::Version(versionName)) // there's a new version
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
			if (versionName.isEmpty())
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
QVariant QuickModInstanceModList::headerData(int section, Qt::Orientation orientation,
											 int role) const
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

QMap<QuickModUid, QString> QuickModInstanceModList::quickmods() const
{
	if (m_type == ResourcePacks || m_type == CoreMods || m_type == TexturePacks)
	{
		return QMap<QuickModUid, QString>();
	}
	QMap<QuickModUid, QString> out;
	auto temp_instance = dynamic_cast<OneSixInstance *>(m_instance);
	auto mods =
		temp_instance->getFullVersion()->quickmods;
	for (auto it = mods.begin(); it != mods.end(); ++it)
	{
		out.insert(QuickModUid(it.key()), it.value());
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
	if (quickmods()[uid].isEmpty())
	{
		return MMC->quickmodslist()->mods(uid).first();
	}
	return MMC->quickmodslist()->modVersion(uid, quickmods()[uid])->mod;
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

void QuickModInstanceModList::updateMod(const QModelIndex &index)
{
	if (isModListArea(index))
	{
		return;
	}
	// TODO allow updating in bulk
	dynamic_cast<OneSixInstance *>(m_instance)
		->setQuickModVersion(modAt(index.row())->uid(), QString());
}

void QuickModInstanceModList::removeMod(const QModelIndex &index)
{
	if (isModListArea(index))
	{
		return;
	}
	auto mod = modAt(index.row());
	auto instance = dynamic_cast<OneSixInstance *>(m_instance);
	if (auto version = mod->version(quickmods()[mod->uid()]))
	{
		QuickModLibraryInstaller(version).remove(instance);
	}
	instance->removeQuickMod(mod->uid());
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
		return true;
		// FIXME: broken code ahead!
		const QString file = index.data(ModList::ModFileRole).toString();
		for (auto it = m_list->quickmods().begin(); it != m_list->quickmods().end(); ++it)
		{
			const QuickModUid uid = it.key();
			if (!uid.isValid() || !MMC->quickmodslist()->mods(uid).isEmpty())
			{
				continue;
			}
			auto mod = MMC->quickmodslist()->mods(uid).first();
			if (!mod)
			{
				continue;
			}
			const QString existingFile =
				MMC->quickmodslist()->installedModFiles(mod, m_list->m_instance)[it.value()];
			if (file == existingFile)
			{
				return false;
			}
		}
	}
	return true;
}
