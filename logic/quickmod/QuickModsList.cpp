#include "QuickModsList.h"

#include <QMimeData>
#include <QIcon>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "logic/net/CacheDownload.h"
#include "logic/net/NetJob.h"
#include "QuickModFilesUpdater.h"
#include "QuickMod.h"
#include "QuickModVersion.h"
#include "logic/Mod.h"
#include "logic/BaseInstance.h"
#include "depends/settings/setting.h"
#include "MultiMC.h"
#include "logic/lists/InstanceList.h"

#include "depends/settings/inisettingsobject.h"

// TODO updating of mods

QuickModsList::QuickModsList(QObject *parent)
	: QAbstractListModel(parent), m_updater(new QuickModFilesUpdater(this)),
	  m_settings(new INISettingsObject("quickmod.cfg", this))
{
	m_settings->registerSetting("AvailableMods",
								QVariant::fromValue(QMap<QString, QMap<QString, QString>>()));
	m_settings->registerSetting("TrustedWebsites", QVariantList());

	connect(m_updater, &QuickModFilesUpdater::error, this, &QuickModsList::error);
}

QuickModsList::~QuickModsList()
{
}

QHash<int, QByteArray> QuickModsList::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[NameRole] = "name";
	roles[WebsiteRole] = "website";
	roles[IconRole] = "icon";
	roles[LogoRole] = "logo";
	roles[UpdateRole] = "update";
	roles[ReferencesRole] = "recommendedUrls";
	roles[DependentUrlsRole] = "dependentUrls";
	roles[NemNameRole] = "nemName";
	roles[ModIdRole] = "modId";
	roles[CategoriesRole] = "categories";
	roles[TagsRole] = "tags";
	return roles;
}

int QuickModsList::rowCount(const QModelIndex &) const
{
	return m_mods.size(); // <-----
}
Qt::ItemFlags QuickModsList::flags(const QModelIndex &index) const
{
	return Qt::ItemIsDropEnabled | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant QuickModsList::data(const QModelIndex &index, int role) const
{
	if (0 > index.row() || index.row() >= m_mods.size())
	{
		return QVariant();
	}

	QuickMod *mod = m_mods.at(index.row());

	switch (role)
	{
	case Qt::DisplayRole:
		return mod->name();
	case Qt::DecorationRole:
		if (mod->icon().isNull())
		{
			return QColor(0, 0, 0, 0);
		}
		else
		{
			return mod->icon();
		}
	case Qt::ToolTipRole:
		return mod->description();
	case NameRole:
		return mod->name();
	case DescriptionRole:
		return mod->description();
	case WebsiteRole:
		return mod->websiteUrl();
	case IconRole:
		return mod->icon();
	case LogoRole:
		return mod->logo();
	case UpdateRole:
		return mod->updateUrl();
	case ReferencesRole:
		return QVariant::fromValue(mod->references());
	case NemNameRole:
		return mod->nemName();
	case ModIdRole:
		return mod->modId();
	case CategoriesRole:
		return mod->categories();
	case TagsRole:
		return mod->tags();
	case QuickModRole:
		return QVariant::fromValue(mod);
	case IsStubRole:
		return mod->isStub();
	}

	return QVariant();
}

bool QuickModsList::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row,
									int column, const QModelIndex &parent) const
{
	if (action != Qt::CopyAction)
	{
		return false;
	}
	return data->hasText() | data->hasUrls();
}
bool QuickModsList::dropMimeData(const QMimeData *data, Qt::DropAction action, int row,
								 int column, const QModelIndex &parent)
{
	if (!canDropMimeData(data, action, row, column, parent))
	{
		return false;
	}

	if (data->hasText())
	{
		registerMod(data->text().toUtf8());
	}
	else if (data->hasUrls())
	{
		foreach(const QUrl & url, data->urls())
		{
			registerMod(url);
		}
	}
	else
	{
		return false;
	}

	return true;
}
Qt::DropActions QuickModsList::supportedDropActions() const
{
	return Qt::CopyAction;
}
Qt::DropActions QuickModsList::supportedDragActions() const
{
	return 0;
}

QuickMod *QuickModsList::modForModId(const QString &modId) const
{
	if (modId.isEmpty())
	{
		return 0;
	}
	foreach (QuickMod *mod, m_mods)
	{
		if (mod->modId() == modId)
		{
			return mod;
		}
	}

	return 0;
}
QuickMod *QuickModsList::mod(const QString &uid) const
{
	foreach (QuickMod *mod, m_mods)
	{
		if (mod->uid() == uid)
		{
			return mod;
		}
	}

	return 0;
}
QuickModVersionPtr QuickModsList::modVersion(const QString &modUid, const QString &versionName) const
{
	QuickMod *m = mod(modUid);
	if (m)
	{
		return m->version(versionName);
	}
	else
	{
		return 0;
	}
}

void QuickModsList::modAddedBy(const Mod &mod, BaseInstance *instance)
{
	QuickMod *qMod = m_updater->ensureExists(mod);
	markModAsInstalled(qMod, QuickModVersion::invalid(qMod), mod.filename().absoluteFilePath(),
					   instance);
}
void QuickModsList::modRemovedBy(const Mod &mod, BaseInstance *instance)
{
	QuickMod *qMod = m_updater->ensureExists(mod);
	markModAsUninstalled(qMod, QuickModVersion::invalid(qMod), instance);
}

void QuickModsList::markModAsExists(QuickMod *mod, const BaseVersionPtr version,
									const QString &fileName)
{
	auto mods = m_settings->getSetting("AvailableMods")->get().toMap();
	auto map = mods[mod->uid()].toMap();
	map[version->name()] = fileName;
	mods[mod->uid()] = map;
	m_settings->getSetting("AvailableMods")->set(QVariant(mods));
}

void QuickModsList::markModAsInstalled(QuickMod *mod, const BaseVersionPtr version,
									   const QString &fileName, BaseInstance *instance)
{
	auto mods = instance->settings().getSetting("InstalledMods")->get().toMap();
	auto map = mods[mod->uid()].toMap();
	map[version->name()] = fileName;
	mods[mod->uid()] = map;
	instance->settings().getSetting("InstalledMods")->set(QVariant(mods));
}
void QuickModsList::markModAsUninstalled(QuickMod *mod, const BaseVersionPtr version,
										 BaseInstance *instance)
{
	auto mods = instance->settings().getSetting("InstalledMods")->get().toMap();
	auto map = mods[mod->uid()].toMap();
	map.remove(version->name());
	if (map.isEmpty())
	{
		mods.remove(mod->uid());
	}
	else
	{
		mods[mod->uid()] = map;
	}
	instance->settings().set("InstalledMods", QVariant(mods));
}
bool QuickModsList::isModMarkedAsInstalled(QuickMod *mod, const BaseVersionPtr version,
										   BaseInstance *instance) const
{
	auto mods = instance->settings().getSetting("InstalledMods")->get().toMap();
	return mods.contains(mod->uid()) &&
		   mods.value(mod->uid()).toMap().contains(version->name());
}
bool QuickModsList::isModMarkedAsExists(QuickMod *mod, const BaseVersionPtr version) const
{
	return isModMarkedAsExists(mod, version->name());
}
bool QuickModsList::isModMarkedAsExists(QuickMod *mod, const QString &version) const
{
	auto mods = m_settings->getSetting("AvailableMods")->get().toMap();
	return mods.contains(mod->uid()) &&
			mods.value(mod->uid()).toMap().contains(version);
}
QString QuickModsList::installedModFile(QuickMod *mod, const BaseVersionPtr version,
										BaseInstance *instance) const
{
	if (!isModMarkedAsInstalled(mod, version, instance))
	{
		return QString();
	}
	auto mods = instance->settings().getSetting("InstalledMods")->get().toMap();
	return mods[mod->uid()].toMap()[version->name()].toString();
}
QString QuickModsList::existingModFile(QuickMod *mod, const BaseVersionPtr version) const
{
	if (!isModMarkedAsExists(mod, version))
	{
		return QString();
	}
	auto mods = m_settings->getSetting("AvailableMods")->get().toMap();
	return mods[mod->uid()].toMap()[version->name()].toString();
}

bool QuickModsList::isWebsiteTrusted(const QUrl &url) const
{
	auto websites = m_settings->getSetting("TrustedWebsites")->get().toList();
	return websites.contains(url.toString());
}
void QuickModsList::setWebsiteTrusted(const QUrl &url, const bool trusted)
{
	auto websites = m_settings->getSetting("TrustedWebsites")->get().toList();
	if (trusted && !websites.contains(url.toString()))
	{
		websites.append(url.toString());
	}
	else if (!trusted)
	{
		websites.removeAll(url.toString());
	}
	m_settings->getSetting("TrustedWebsites")->set(websites);
}

void QuickModsList::registerMod(const QString &fileName)
{
	registerMod(QUrl::fromLocalFile(fileName));
}
void QuickModsList::registerMod(const QUrl &url)
{
	m_updater->registerFile(url);
}

void QuickModsList::updateFiles()
{
	m_updater->update();
}

void QuickModsList::addMod(QuickMod *mod)
{
	connect(mod, &QuickMod::iconUpdated, this, &QuickModsList::modIconUpdated);
	connect(mod, &QuickMod::logoUpdated, this, &QuickModsList::modLogoUpdated);

	for (int i = 0; i < m_mods.size(); ++i)
	{
		if (m_mods.at(i)->compare(mod))
		{
			disconnect(m_mods.at(i), &QuickMod::iconUpdated, this,
					   &QuickModsList::modIconUpdated);
			disconnect(m_mods.at(i), &QuickMod::logoUpdated, this,
					   &QuickModsList::modLogoUpdated);
			m_mods.replace(i, mod);

			emit modAdded(mod);
			emit modsListChanged();

			return;
		}
	}

	beginInsertRows(QModelIndex(), 0, 0);
	m_mods.prepend(mod);
	endInsertRows();

	emit modAdded(mod);
	emit modsListChanged();
}
void QuickModsList::clearMods()
{
	beginResetModel();
	qDeleteAll(m_mods);
	m_mods.clear();
	endResetModel();

	emit modsListChanged();
}
void QuickModsList::removeMod(QuickMod *mod)
{
	if (!m_mods.contains(mod))
	{
		return;
	}

	disconnect(mod, &QuickMod::iconUpdated, this, &QuickModsList::modIconUpdated);
	disconnect(mod, &QuickMod::logoUpdated, this, &QuickModsList::modLogoUpdated);

	beginRemoveRows(QModelIndex(), m_mods.indexOf(mod), m_mods.indexOf(mod));
	m_mods.takeAt(m_mods.indexOf(mod));
	endRemoveRows();

	emit modsListChanged();
}

void QuickModsList::modIconUpdated()
{
	auto mod = qobject_cast<QuickMod *>(sender());
	auto modIndex = index(m_mods.indexOf(mod), 0);
	emit dataChanged(modIndex, modIndex, QVector<int>() << Qt::DecorationRole << IconRole);
}
void QuickModsList::modLogoUpdated()
{
	auto mod = qobject_cast<QuickMod *>(sender());
	auto modIndex = index(m_mods.indexOf(mod), 0);
	emit dataChanged(modIndex, modIndex, QVector<int>() << LogoRole);
}

void QuickModsList::unregisterMod(QuickMod *mod)
{
	int row = m_mods.indexOf(mod);
	if (row == -1)
	{
		return;
	}
	beginRemoveRows(QModelIndex(), row, row);
	m_mods.removeAt(row);
	endRemoveRows();
	m_updater->unregisterMod(mod);
}
