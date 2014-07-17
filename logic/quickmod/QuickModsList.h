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
#include <QUrl>
#include <QStringList>
#include <QIcon>
#include <QPixmap>
#include <QCryptographicHash>
#include <logic/quickmod/QuickModVersion.h>
#include <logic/BaseInstance.h>
#include <logic/quickmod/QuickMod.h>

class QuickModFilesUpdater;
class Mod;
class SettingsObject;

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
		NemNameRole,
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
	QuickModPtr modAt(const int index) const
	{
		return m_mods[index];
	}

	QuickModPtr modForModId(const QString &modId) const;
	QList<QuickModPtr> mods(const QuickModUid &uid) const;
	QuickModVersionPtr modVersion(const QuickModUid &modUid, const QString &versionName) const;
	QuickModVersionPtr latestVersion(const QuickModUid &modUid, const QString &mcVersion) const;

	void markModAsExists(QuickModPtr mod, const BaseVersionPtr version,
						 const QString &fileName);
	void markModAsInstalled(QuickModPtr mod, const BaseVersionPtr version,
							const QString &fileName, InstancePtr instance);
	void markModAsUninstalled(QuickModPtr mod, const BaseVersionPtr version,
							  InstancePtr instance);
	bool isModMarkedAsInstalled(QuickModPtr mod, const BaseVersionPtr version,
								InstancePtr instance) const;
	bool isModMarkedAsExists(QuickModPtr mod, const BaseVersionPtr version) const;
	bool isModMarkedAsExists(QuickModPtr mod, const QString &version) const;
	QMap<QString, QString> installedModFiles(QuickModPtr mod, BaseInstance *instance) const;
	QString existingModFile(QuickModPtr mod, const BaseVersionPtr version) const;
	QString existingModFile(QuickModPtr mod, const QString &version) const;

	void setRepositoryIndexUrl(const QString &repository, const QUrl &url);
	QUrl repositoryIndexUrl(const QString &repository) const;
	bool haveRepositoryIndexUrl(const QString &repository) const;
	QList<QUrl> indices() const;

	bool haveUid(const QuickModUid &uid) const;

	QList<QuickModUid> updatedModsForInstance(std::shared_ptr<BaseInstance> instance) const;

	void releaseFromSandbox(QuickModPtr mod);

public
slots:
	void registerMod(const QString &fileName);
	void registerMod(const QUrl &url, bool sandbox);
	void unregisterMod(QuickModPtr mod);

	void updateFiles();

private
slots:
	void touchMod(QuickModPtr mod);
	void addMod(QuickModPtr mod);
	void clearMods();
	void removeMod(QuickModPtr mod);

	void modIconUpdated();
	void modLogoUpdated();

	void cleanup();

signals:
	void sandboxModAdded(QuickModPtr mod);
	void modAdded(QuickModPtr mod);
	void modsListChanged();
	void error(const QString &message);

private:
	/// Gets the index of the given mod in the list.
	int getQMIndex(QuickModPtr mod) const;
	QuickModPtr getQMPtr(QuickMod *mod) const;

	friend class QuickModFilesUpdater;
	QuickModFilesUpdater *m_updater;

	QList<QuickModPtr> m_mods;

	SettingsObject *m_settings;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QuickModsList::Flags)
