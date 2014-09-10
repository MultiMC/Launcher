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
#include "QuickModBaseDownloadAction.h"
#include "QuickModVersion.h"
#include "QuickModSettings.h"
#include "logic/Mod.h"
#include "logic/BaseInstance.h"
#include "logic/OneSixInstance.h"
#include "logic/settings/Setting.h"
#include "MultiMC.h"
#include "logic/InstanceList.h"
#include "modutils.h"

#include "logic/settings/INISettingsObject.h"

QuickModsList::QuickModsList(const Flags flags, QObject *parent)
	: QAbstractListModel(parent)
{
	if (!flags.testFlag(DontCleanup))
	{
		cleanup();
	}
}

QuickModsList::~QuickModsList()
{
}

int QuickModsList::getQMIndex(QuickModMetadataPtr mod) const
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

QuickModMetadataPtr QuickModsList::getQMPtr(QuickModMetadata *mod) const
{
	for (auto m : m_mods)
	{
		if (m.get() == mod)
		{
			return m;
		}
	}
	return QuickModMetadataPtr();
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

	QuickModMetadataPtr mod = m_mods.at(index.row());

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
	case ModIdRole:
		return mod->modId();
	case CategoriesRole:
		return mod->categories();
	case MCVersionsRole:
		return QStringList();//FIXME mod->mcVersions();
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
		registerMod(QUrl(data->text()));
	}
	else if (data->hasUrls())
	{
		for (const QUrl &url : data->urls())
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

QuickModMetadataPtr QuickModsList::modForModId(const QString &modId) const
{
	if (modId.isEmpty())
	{
		return 0;
	}
	for (QuickModMetadataPtr mod : m_mods)
	{
		if (mod->modId() == modId)
		{
			return mod;
		}
	}

	return 0;
}
QList<QuickModMetadataPtr> QuickModsList::mods(const QuickModRef &uid) const
{
	// TODO repository priority
	QList<QuickModMetadataPtr> out;
	for (QuickModMetadataPtr mod : m_mods)
	{
		if (mod->uid() == uid)
		{
			out.append(mod);
		}
	}
	return out;
}

QuickModMetadataPtr QuickModsList::mod(const QString &internalUid) const
{
	for (QuickModMetadataPtr mod : m_mods)
	{
		if (mod->internalUid() == internalUid)
		{
			return mod;
		}
	}
	return nullptr;
}

QList<QuickModVersionRef> QuickModsList::modsProvidingModVersion(const QuickModRef &uid,
																 const QuickModVersionRef &version) const
{
	QList<QuickModVersionRef> out;
	for (auto mod : m_mods)
	{
		for (QuickModVersionRef versionRef : QList<QuickModVersionRef>())//FIXME mod->versions())
		{
			QuickModVersionPtr versionObj = versionRef.findVersion();
			if (versionObj->provides.contains(uid) &&
				Util::versionIsInInterval(versionObj->provides.value(uid).toString(), version.toString()))
			{
				// this mod provides exactly the needed version :)
				out.append(versionObj->version());
			}
		}
	}
	return out;
}

QuickModVersionRef QuickModsList::latestVersion(const QuickModRef &modUid,
												const QString &mcVersion) const
{
	QuickModVersionRef latest;
	for (auto mod : mods(modUid))
	{
		auto modLatest = QuickModVersionRef();//FIXME mod->latestVersion(mcVersion);
		if (!latest.isValid())
		{
			latest = modLatest;
		}
		else if (modLatest.isValid() && modLatest > latest)
		{
			latest = modLatest;
		}
	}
	return latest;
}

QList<QuickModRef> QuickModsList::updatedModsForInstance(std::shared_ptr<OneSixInstance> instance)
	const
{
	QList<QuickModRef> mods;
	for (auto it = instance->getFullVersion()->quickmods.begin();
		 it != instance->getFullVersion()->quickmods.end(); ++it)
	{
		if (!it.value().first.isValid())
		{
			continue;
		}
		auto latest = latestVersion(it.key(), instance->intendedVersionId());
		if (!latest.isValid())
		{
			continue;
		}
		if (it.value().first < latest)
		{
			mods.append(QuickModRef(it.key()));
		}
	}
	return mods;
}

void QuickModsList::registerMod(const QString &fileName)
{
	registerMod(QUrl::fromLocalFile(fileName));
}
void QuickModsList::registerMod(const QUrl &url)
{
	NetJob *job = new NetJob("QuickMod Download");
	job->addNetAction(QuickModBaseDownloadAction::make(job, url));
	connect(job, &NetJob::succeeded, job, &NetJob::deleteLater);
	connect(job, &NetJob::failed, job, &NetJob::deleteLater);
	job->start();
}

void QuickModsList::updateFiles()
{
	// TODO
}

void QuickModsList::addMod(QuickModMetadataPtr mod)
{
	connect(mod.get(), &QuickModMetadata::iconUpdated, this, &QuickModsList::modIconUpdated);
	connect(mod.get(), &QuickModMetadata::logoUpdated, this, &QuickModsList::modLogoUpdated);

	for (int i = 0; i < m_mods.size(); ++i)
	{
		if (m_mods.at(i)->compare(mod))
		{
			disconnect(m_mods.at(i).get(), &QuickModMetadata::iconUpdated, this,
					   &QuickModsList::modIconUpdated);
			disconnect(m_mods.at(i).get(), &QuickModMetadata::logoUpdated, this,
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

void QuickModsList::modIconUpdated()
{
	auto modIndex = index(getQMIndex(getQMPtr(qobject_cast<QuickModMetadata *>(sender()))), 0);
	emit dataChanged(modIndex, modIndex, QVector<int>() << Qt::DecorationRole << IconRole);
}
void QuickModsList::modLogoUpdated()
{
	auto modIndex = index(getQMIndex(getQMPtr(qobject_cast<QuickModMetadata *>(sender()))), 0);
	emit dataChanged(modIndex, modIndex, QVector<int>() << LogoRole);
}

void QuickModsList::cleanup()
{
	// ensure that only mods that really exist on the local filesystem are marked as available
	QDir dir;
	auto mods = MMC->quickmodSettings()->settings()->get("AvailableMods").toMap();
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
	MMC->quickmodSettings()->settings()->set("AvailableMods", mods);
}

void QuickModsList::unregisterMod(QuickModMetadataPtr mod)
{
	if (!m_mods.contains(mod))
	{
		return;
	}
	disconnect(mod.get(), &QuickModMetadata::iconUpdated, this, &QuickModsList::modIconUpdated);
	disconnect(mod.get(), &QuickModMetadata::logoUpdated, this, &QuickModsList::modLogoUpdated);

	beginRemoveRows(QModelIndex(), m_mods.indexOf(mod), m_mods.indexOf(mod));
	m_mods.removeAll(mod);
	endRemoveRows();
	// FIXME actually remove the mod

	emit modsListChanged();
}
