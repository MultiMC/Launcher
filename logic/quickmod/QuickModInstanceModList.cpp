#include "QuickModInstanceModList.h"

#include "MultiMC.h"
#include "QuickModsList.h"
#include "modutils.h"

QuickModInstanceModList::QuickModInstanceModList(OneSixInstance *instance, std::shared_ptr<ModList> modList, QObject *parent)
	: QAbstractListModel(parent), m_instance(instance), m_modList(modList)
{
	connect(m_modList.get(), &ModList::modelAboutToBeReset, this, &QuickModInstanceModList::beginResetModel);
	connect(m_modList.get(), &ModList::modelReset, this, &QuickModInstanceModList::endResetModel);
	connect(m_modList.get(), &ModList::rowsAboutToBeInserted, [this](const QModelIndex &parent, const int first, const int last)
	{
		beginInsertRows(parent, first + quickmods().size(), last + quickmods().size());
	});
	connect(m_modList.get(), &ModList::rowsInserted, this, &QuickModInstanceModList::endInsertRows);
	connect(m_modList.get(), &ModList::dataChanged, [this](const QModelIndex &tl, const QModelIndex &br, const QVector<int> &roles)
	{
		emit dataChanged(mapFromModList(tl), mapFromModList(br), roles);
	});
	connect(m_instance, &OneSixInstance::versionReloaded, this, &QuickModInstanceModList::resetModel);

	connect(MMC->quickmodslist().get(), &QuickModsList::rowsInserted, [this](const QModelIndex &parent, const int first, const int last)
	{
		for (int i = first; i < (last + 1); ++i)
		{
			auto mod = MMC->quickmodslist()->modAt(i);
			connect(mod.get(), &QuickMod::versionsUpdated, this, &QuickModInstanceModList::quickmodVersionUpdated);
			connect(mod.get(), &QuickMod::iconUpdated, this, &QuickModInstanceModList::quickmodIconUpdated);
		}
	});
	connect(MMC->quickmodslist().get(), &QuickModsList::rowsAboutToBeRemoved, [this](const QModelIndex &parent, const int first, const int last)
	{
		for (int i = first; i < (last + 1); ++i)
		{
			disconnect(MMC->quickmodslist()->modAt(i).get(), 0, this, 0);
		}
	});
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
				const QString latest = mod->latestVersion(m_instance->intendedVersionId())->name();
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
		return mod->uid();
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
QVariant QuickModInstanceModList::headerData(int section, Qt::Orientation orientation, int role) const
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
bool QuickModInstanceModList::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
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

void QuickModInstanceModList::quickmodVersionUpdated()
{
	emit dataChanged(index(0, VersionColumn), index(rowCount(), VersionColumn), QVector<int>() << Qt::DecorationRole << Qt::DisplayRole);
}
void QuickModInstanceModList::quickmodIconUpdated()
{
	emit dataChanged(index(0, NameColumn), index(rowCount(), NameColumn), QVector<int>() << Qt::DecorationRole);
}

QMap<QString, QString> QuickModInstanceModList::quickmods() const
{
	return m_instance->getFullVersion()->quickmods;
}

QuickModPtr QuickModInstanceModList::modAt(const int row) const
{
	return MMC->quickmodslist()->mod(quickmods().keys()[row]);
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

void QuickModInstanceModList::scheduleModForUpdate(const QModelIndex &index)
{
	if (isModListArea(index))
	{
		return;
	}
	// TODO allow updating in bulk
	m_instance->setQuickModVersion(modAt(index.row())->uid(), QString());
}

void QuickModInstanceModList::scheduleModForRemoval(const QModelIndex &index)
{
	if (isModListArea(index))
	{
		return;
	}
	// TODO allow removing in bulk
	m_instance->removeQuickMod(modAt(index.row())->uid());
}

QuickModInstanceModListProxy::QuickModInstanceModListProxy(QuickModInstanceModList *list, QObject *parent)
	: QSortFilterProxyModel(parent), m_list(list)
{
	setSourceModel(m_list);
}

bool QuickModInstanceModListProxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	const QModelIndex index = m_list->index(source_row, 0, source_parent);
	if (m_list->isModListArea(index))
	{
		return true;
		// FIXME: broken code ahead!
		const QString file = index.data(ModList::ModFileRole).toString();
		for (auto it = m_list->quickmods().begin(); it != m_list->quickmods().end(); ++it)
		{
			const QString uid = it.key();
			if (uid.isEmpty())
			{
				continue;
			}
			auto mod = MMC->quickmodslist()->mod(uid);
			if (!mod)
			{
				continue;
			}
			const QString existingFile = MMC->quickmodslist()->installedModFiles(mod, m_list->m_instance)[it.value()];
			if (file == existingFile)
			{
				return false;
			}
		}
	}
	return true;
}
