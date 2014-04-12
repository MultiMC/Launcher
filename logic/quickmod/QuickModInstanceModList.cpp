#include "QuickModInstanceModList.h"

#include "MultiMC.h"
#include "QuickModsList.h"

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
		emit dataChanged(modListIndexToExternal(tl), modListIndexToExternal(br), roles);
	});
	connect(m_instance, &OneSixInstance::versionReloaded, [this](){beginResetModel();endResetModel();});
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
		return m_modList->data(externalIndexToModList(index), role);
	}

	const int row = index.row();
	const int col = index.column();

	QuickMod *mod = MMC->quickmodslist()->mod(quickmods().keys()[row]);
	if (!mod)
	{
		return QVariant();
	}

	QuickModVersionPtr version = mod->version(quickmods()[mod->uid()]);

	switch (role)
	{
	case Qt::DecorationRole:
		switch (col)
		{
		case NameColumn:
			return mod->icon();
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
			if (!version)
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
		return m_modList->flags(externalIndexToModList(index));
	}
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool QuickModInstanceModList::setData(const QModelIndex &index, const QVariant &value, int role)
{
	return m_modList->setData(externalIndexToModList(index), value, role);
}
QMimeData *QuickModInstanceModList::mimeData(const QModelIndexList &indexes) const
{
	QModelIndexList i;
	for (auto index : indexes)
	{
		i.append(externalIndexToModList(index));
	}
	return m_modList->mimeData(i);
}
QStringList QuickModInstanceModList::mimeTypes() const
{
	return m_modList->mimeTypes();
}
bool QuickModInstanceModList::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	return m_modList->dropMimeData(data, action, row - quickmods().size(), column, parent);
}
Qt::DropActions QuickModInstanceModList::supportedDragActions() const
{
	return m_modList->supportedDragActions();
}
Qt::DropActions QuickModInstanceModList::supportedDropActions() const
{
	return m_modList->supportedDropActions();
}

QMap<QString, QString> QuickModInstanceModList::quickmods() const
{
	return m_instance->getFullVersion()->quickmods;
}

QModelIndex QuickModInstanceModList::externalIndexToModList(const QModelIndex &index) const
{
	return m_modList->index(index.row() - quickmods().size(), index.column(), QModelIndex());
}
QModelIndex QuickModInstanceModList::modListIndexToExternal(const QModelIndex &index) const
{
	return this->index(index.row() + quickmods().size(), index.column(), QModelIndex());
}
bool QuickModInstanceModList::isModListArea(const QModelIndex &index) const
{
	return index.row() >= quickmods().size();
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
