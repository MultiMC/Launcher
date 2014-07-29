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
#include "logic/OneSixInstance.h"
#include "logic/settings/Setting.h"
#include "MultiMC.h"
#include "logic/InstanceList.h"
#include "modutils.h"

#include "logic/settings/INISettingsObject.h"

QuickModsList::QuickModsList(const Flags flags, QObject *parent)
	: QAbstractListModel(parent), m_updater(new QuickModFilesUpdater(this)),
	  m_settings(new INISettingsObject(QDir::current().absoluteFilePath("quickmod.cfg"), this))
{
	m_settings->registerSetting("AvailableMods",
								QVariant::fromValue(QMap<QString, QMap<QString, QString>>()));
	m_settings->registerSetting("TrustedWebsites", QVariantList());
	m_settings->registerSetting("Indices", QVariantMap());

	connect(m_updater, &QuickModFilesUpdater::error, this, &QuickModsList::error);
	connect(m_updater, &QuickModFilesUpdater::addedSandboxedMod, this, &QuickModsList::sandboxModAdded);

	if (!flags.testFlag(DontCleanup))
	{
		cleanup();
	}
}

QuickModsList::~QuickModsList()
{
	delete m_settings;
}

int QuickModsList::getQMIndex(QuickModPtr mod) const
{
	for (int i = 0; i < m_mods.count(); i++)
	{
		if (mod == m_mods[i])
		{
			return i;
		}
	}
	return -1;
}

QuickModPtr QuickModsList::getQMPtr(QuickMod *mod) const
{
	for (auto m : m_mods)
	{
		if (m.get() == mod)
		{
			return m;
		}
	}
	return QuickModPtr();
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

	QuickModPtr mod = m_mods.at(index.row());

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
	case UidRole:
		return QVariant::fromValue(mod->uid());
	case RepoRole:
		return mod->repo();
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
	case MCVersionsRole:
		return mod->mcVersions();
	case TagsRole:
		return mod->tags();
	case QuickModRole:
		return QVariant::fromValue(mod);
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
		registerMod(Util::expandQMURL(data->text()), false);
	}
	else if (data->hasUrls())
	{
		for (const QUrl &url : data->urls())
		{
			registerMod(url, false);
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

QuickModPtr QuickModsList::modForModId(const QString &modId) const
{
	if (modId.isEmpty())
	{
		return 0;
	}
	for (QuickModPtr mod : m_mods)
	{
		if (mod->modId() == modId)
		{
			return mod;
		}
	}

	return 0;
}
QList<QuickModPtr> QuickModsList::mods(const QuickModUid &uid) const
{
	// TODO repository priority
	QList<QuickModPtr> out;
	for (QuickModPtr mod : m_mods)
	{
		if (mod->uid() == uid)
		{
			out.append(mod);
		}
	}
	return out;
}
QuickModVersionPtr QuickModsList::modVersion(const QuickModUid &modUid,
											 const QString &versionName) const
{
	for (auto mod : mods(modUid))
	{
		if (auto v = mod->version(versionName))
		{
			return v;
		}
	}
	return 0;
}

QuickModVersionPtr QuickModsList::latestVersion(const QuickModUid &modUid,
												const QString &mcVersion) const
{
	QuickModVersionPtr latest;
	for (auto mod : mods(modUid))
	{
		auto modLatest = mod->latestVersion(mcVersion);
		if (!latest)
		{
			latest = modLatest;
		}
		else if (modLatest && Util::Version(modLatest->name()) > Util::Version(latest->name()))
		{
			latest = modLatest;
		}
	}
	return latest;
}

void QuickModsList::markModAsExists(QuickModPtr mod, const BaseVersionPtr version,
									const QString &fileName)
{
	auto mods = m_settings->get("AvailableMods").toMap();
	auto map = mods[mod->internalUid()].toMap();
	map[version->name()] = fileName;
	mods[mod->internalUid()] = map;
	m_settings->getSetting("AvailableMods")->set(QVariant(mods));
}

void QuickModsList::markModAsInstalled(const QuickModUid uid, const BaseVersionPtr version,
									   const QString &fileName, InstancePtr instance)
{
	auto mods = instance->settings().get("InstalledMods").toMap();
	auto map = mods[uid.toString()].toMap();
	map[version->name()] = fileName;
	mods[uid.toString()] = map;
	instance->settings().getSetting("InstalledMods")->set(QVariant(mods));
}
void QuickModsList::markModAsUninstalled(const QuickModUid uid, const BaseVersionPtr version,
										 InstancePtr instance)
{
	auto mods = instance->settings().get("InstalledMods").toMap();
	if (version)
	{
		auto map = mods[uid.toString()].toMap();
		map.remove(version->name());
		if (map.isEmpty())
		{
			mods.remove(uid.toString());
		}
		else
		{
			mods[uid.toString()] = map;
		}
	}
	else
	{
		mods.remove(uid.toString());
	}
	instance->settings().set("InstalledMods", QVariant(mods));
}
bool QuickModsList::isModMarkedAsInstalled(const QuickModUid uid, const BaseVersionPtr version,
										   InstancePtr instance) const
{
	auto mods = instance->settings().get("InstalledMods").toMap();
	if (!version)
	{
		return mods.contains(uid.toString());
	}
	return mods.contains(uid.toString()) &&
		   mods.value(uid.toString()).toMap().contains(version->name());
}
bool QuickModsList::isModMarkedAsExists(QuickModPtr mod, const BaseVersionPtr version) const
{
	if (!version)
	{
		return m_settings->get("AvailableMods").toMap().contains(mod->internalUid());
	}
	return isModMarkedAsExists(mod, version->name());
}
bool QuickModsList::isModMarkedAsExists(QuickModPtr mod, const QString &version) const
{
	auto mods = m_settings->get("AvailableMods").toMap();
	return mods.contains(mod->internalUid()) &&
		   mods.value(mod->internalUid()).toMap().contains(version);
}
QMap<QString, QString> QuickModsList::installedModFiles(const QuickModUid uid,
														BaseInstance *instance) const
{
	auto mods = instance->settings().get("InstalledMods").toMap();
	auto tmp = mods[uid.toString()].toMap();
	QMap<QString, QString> out;
	for (auto it = tmp.begin(); it != tmp.end(); ++it)
	{
		out.insert(it.key(), it.value().toString());
	}
	return out;
}
QString QuickModsList::existingModFile(QuickModPtr mod, const BaseVersionPtr version) const
{
	return existingModFile(mod, version->name());
}
QString QuickModsList::existingModFile(QuickModPtr mod, const QString &version) const
{
	if (!isModMarkedAsExists(mod, version))
	{
		return QString();
	}
	auto mods = m_settings->get("AvailableMods").toMap();
	return mods[mod->internalUid()].toMap()[version].toString();
}

void QuickModsList::setRepositoryIndexUrl(const QString &repository, const QUrl &url)
{
	QMap<QString, QVariant> map = m_settings->get("Indices").toMap();
	map[repository] = url.toString(QUrl::FullyEncoded);
	m_settings->set("Indices", map);
}
QUrl QuickModsList::repositoryIndexUrl(const QString &repository) const
{
	return QUrl(m_settings->get("Indices").toMap()[repository].toString(), QUrl::StrictMode);
}
bool QuickModsList::haveRepositoryIndexUrl(const QString &repository) const
{
	return m_settings->get("Indices").toMap().contains(repository);
}
QList<QUrl> QuickModsList::indices() const
{
	QList<QUrl> out;
	const auto map = m_settings->get("Indices").toMap();
	for (const auto value : map.values())
	{
		out.append(QUrl(value.toString(), QUrl::StrictMode));
	}
	return out;
}

bool QuickModsList::haveUid(const QuickModUid &uid, const QString &repo) const
{
	for (auto mod : m_mods)
	{
		if (mod->uid() == uid && mod->repo() == repo)
		{
			return true;
		}
	}
	return false;
}

QList<QuickModUid>
QuickModsList::updatedModsForInstance(std::shared_ptr<BaseInstance> instance) const
{
	QList<QuickModUid> mods;
	std::shared_ptr<OneSixInstance> onesix =
		std::dynamic_pointer_cast<OneSixInstance>(instance);
	for (auto it = onesix->getFullVersion()->quickmods.begin();
		 it != onesix->getFullVersion()->quickmods.end(); ++it)
	{
		if (it.value().first.isEmpty())
		{
			continue;
		}
		auto latest = latestVersion(it.key(), instance->intendedVersionId());
		if (!latest)
		{
			continue;
		}
		if (Util::Version(it.value().first) < Util::Version(latest->name()))
		{
			mods.append(QuickModUid(it.key()));
		}
	}
	return mods;
}

void QuickModsList::releaseFromSandbox(QuickModPtr mod)
{
	m_updater->releaseFromSandbox(mod);
}

void QuickModsList::registerMod(const QString &fileName)
{
	registerMod(QUrl::fromLocalFile(fileName), false);
}
void QuickModsList::registerMod(const QUrl &url, bool sandbox)
{
	m_updater->registerFile(url, sandbox);
}

void QuickModsList::updateFiles()
{
	m_updater->update();
}

void QuickModsList::touchMod(QuickModPtr mod)
{
	emit modAdded(mod);
}

void QuickModsList::addMod(QuickModPtr mod)
{
	connect(mod.get(), &QuickMod::iconUpdated, this, &QuickModsList::modIconUpdated);
	connect(mod.get(), &QuickMod::logoUpdated, this, &QuickModsList::modLogoUpdated);

	for (int i = 0; i < m_mods.size(); ++i)
	{
		if (m_mods.at(i)->compare(mod))
		{
			disconnect(m_mods.at(i).get(), &QuickMod::iconUpdated, this,
					   &QuickModsList::modIconUpdated);
			disconnect(m_mods.at(i).get(), &QuickMod::logoUpdated, this,
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
	m_mods.clear();
	endResetModel();

	emit modsListChanged();
}
void QuickModsList::removeMod(QuickModPtr mod)
{
	if (!m_mods.contains(mod))
	{
		return;
	}

	disconnect(mod.get(), &QuickMod::iconUpdated, this, &QuickModsList::modIconUpdated);
	disconnect(mod.get(), &QuickMod::logoUpdated, this, &QuickModsList::modLogoUpdated);

	beginRemoveRows(QModelIndex(), m_mods.indexOf(mod), m_mods.indexOf(mod));
	m_mods.takeAt(m_mods.indexOf(mod));
	endRemoveRows();

	emit modsListChanged();
}

void QuickModsList::modIconUpdated()
{
	auto modIndex = index(getQMIndex(getQMPtr(qobject_cast<QuickMod *>(sender()))), 0);
	emit dataChanged(modIndex, modIndex, QVector<int>() << Qt::DecorationRole << IconRole);
}
void QuickModsList::modLogoUpdated()
{
	auto modIndex = index(getQMIndex(getQMPtr(qobject_cast<QuickMod *>(sender()))), 0);
	emit dataChanged(modIndex, modIndex, QVector<int>() << LogoRole);
}

void QuickModsList::cleanup()
{
	// ensure that only mods that really exist on the local filesystem are marked as available
	QDir dir;
	auto mods = m_settings->get("AvailableMods").toMap();
	QMutableMapIterator<QString, QVariant> modIt(mods);
	while (modIt.hasNext())
	{
		auto versions = modIt.next().value().toMap();
		QMutableMapIterator<QString, QVariant> versionIt(versions);
		while (versionIt.hasNext())
		{
			if (!dir.exists(versionIt.next().value().toString()))
			{
				versionIt.remove();
			}
		}
		modIt.setValue(versions);
		if (modIt.value().toMap().isEmpty())
		{
			modIt.remove();
		}
	}
	m_settings->set("AvailableMods", mods);

	m_updater->cleanupSandboxedFiles();
}

void QuickModsList::unregisterMod(QuickModPtr mod)
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
