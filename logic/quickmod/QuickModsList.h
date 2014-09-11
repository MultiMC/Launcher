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

#include <QAbstractListModel>

#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModMetadata.h"
#include "logic/BaseInstance.h"

class QUrl;

class QuickModDatabase;
class Mod;
class SettingsObject;
class OneSixInstance;

class QuickModsList : public QAbstractListModel
{
	Q_OBJECT
public:
	enum Flag
	{
		NoFlags = 0x0,
		DontCleanup = 0x1
	};
	Q_DECLARE_FLAGS(Flags, Flag)

	explicit QuickModsList(const Flags flags = NoFlags, QObject *parent = 0);
	~QuickModsList();

	enum Roles
	{
		NameRole = Qt::UserRole,
		UidRole,
		RepoRole,
		DescriptionRole,
		WebsiteRole,
		IconRole,
		LogoRole,
		UpdateRole,
		ReferencesRole,
		DependentUrlsRole,
		ModIdRole,
		CategoriesRole,
		MCVersionsRole,
		TagsRole,
		QuickModRole
	};
	QHash<int, QByteArray> roleNames() const;

	int rowCount(const QModelIndex &) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	QVariant data(const QModelIndex &index, int role) const;

	bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
						 const QModelIndex &parent) const;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
					  const QModelIndex &parent);
	Qt::DropActions supportedDropActions() const;
	Qt::DropActions supportedDragActions() const;

	int numMods() const
	{
		return m_mods.size();
	}
	QuickModMetadataPtr modAt(const int index) const
	{
		// TODO repository priority
		return m_mods[m_uids[index]].first();
	}

	QList<QuickModMetadataPtr> mods(const QuickModRef &uid) const;
	QuickModMetadataPtr mod(const QuickModRef &uid) const;
	QuickModMetadataPtr mod(const QString &internalUid) const;
	QuickModVersionPtr version(const QuickModVersionRef &version) const;
	QuickModVersionRef latestVersion(const QuickModRef &modUid, const QString &mcVersion) const;
	QStringList minecraftVersions(const QuickModRef &uid) const;
	QList<QuickModVersionRef> versions(const QuickModRef &uid, const QString &mcVersion) const;

	QList<QuickModRef> updatedModsForInstance(std::shared_ptr<OneSixInstance> instance) const;

	bool haveUid(const QuickModRef &uid, const QString &repo = QString()) const;

	/// \internal
	inline QHash<QuickModRef, QList<QuickModMetadataPtr>> quickmods() const
	{
		return m_mods;
	}
	inline QList<QuickModMetadataPtr> allQuickMods() const
	{
		QList<QuickModMetadataPtr> out;
		for (const auto mods : m_mods)
		{
			out.append(mods);
		}
		return out;
	}

public slots:
	void registerMod(const QString &fileName);
	void registerMod(const QUrl &url);

	void updateFiles();

public:
	// called from QuickModDownloadAction
	void addMod(QuickModMetadataPtr mod);
	void addVersion(QuickModVersionPtr version);

	void modIconUpdated();
	void modLogoUpdated();

	void cleanup();

signals:
	void modsListChanged();
	void error(const QString &message);

private:
	/// Gets the index of the given mod in the list.
	int getQMIndex(QuickModMetadataPtr mod) const;
	QuickModMetadataPtr getQMPtr(QuickModMetadata *mod) const;

	QHash<QuickModRef, QList<QuickModMetadataPtr>> m_mods;
	QList<QuickModRef> m_uids; // list that stays ordered for the model
	QHash<QuickModRef, QList<QuickModVersionPtr>> m_versions;

	QuickModDatabase *m_storage;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QuickModsList::Flags)
