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

class QuickModFilesUpdater;
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
		return m_mods[index];
	}

	QuickModMetadataPtr modForModId(const QString &modId) const;
	QList<QuickModMetadataPtr> mods(const QuickModRef &uid) const;
	QuickModMetadataPtr mod(const QString &internalUid) const;
	QList<QuickModVersionRef> modsProvidingModVersion(const QuickModRef &uid,
													  const QuickModVersionRef &version) const;
	QuickModVersionRef latestVersion(const QuickModRef &modUid, const QString &mcVersion) const;

	QList<QuickModRef> updatedModsForInstance(std::shared_ptr<OneSixInstance> instance) const;

	/// \internal
	inline QList<QuickModMetadataPtr> quickmods() const
	{
		return m_mods;
	}

public slots:
	void registerMod(const QString &fileName);
	void registerMod(const QUrl &url);
	void unregisterMod(QuickModMetadataPtr mod);

	void updateFiles();

public:
	void addMod(QuickModMetadataPtr mod);
	void clearMods();

	void modIconUpdated();
	void modLogoUpdated();

	void cleanup();

signals:
	void modAdded(QuickModMetadataPtr mod);
	void modsListChanged();
	void error(const QString &message);

private:
	/// Gets the index of the given mod in the list.
	int getQMIndex(QuickModMetadataPtr mod) const;
	QuickModMetadataPtr getQMPtr(QuickModMetadata *mod) const;

	QList<QuickModMetadataPtr> m_mods;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QuickModsList::Flags)
