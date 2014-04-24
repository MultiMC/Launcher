#pragma once

#include <QAbstractListModel>
#include <QUrl>
#include <QStringList>
#include <QIcon>
#include <QPixmap>
#include <QCryptographicHash>
#include <logic/quickmod/QuickModVersion.h>
#include <logic/BaseInstance.h>

class QuickModFilesUpdater;
class QuickMod;
class QuickModVersion;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;
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
		TagsRole,
		QuickModRole,
		IsStubRole
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
	QuickMod *modAt(const int index) const
	{
		return m_mods[index];
	}

	QuickMod *modForModId(const QString &modId) const;
	QuickMod *mod(const QString &uid) const;
	QuickModVersionPtr modVersion(const QString &modUid, const QString &versionName) const;

	void modAddedBy(const Mod &mod, BaseInstance *instance);
	void modRemovedBy(const Mod &mod, BaseInstance *instance);

	void markModAsExists(QuickMod *mod, const BaseVersionPtr version, const QString &fileName);
	void markModAsInstalled(QuickMod *mod, const BaseVersionPtr version,
							const QString &fileName, BaseInstance *instance);
	void markModAsUninstalled(QuickMod *mod, const BaseVersionPtr version,
							  BaseInstance *instance);
	bool isModMarkedAsInstalled(QuickMod *mod, const BaseVersionPtr version,
								BaseInstance *instance) const;
	bool isModMarkedAsExists(QuickMod *mod, const BaseVersionPtr version) const;
	bool isModMarkedAsExists(QuickMod *mod, const QString &version) const;
	QMap<QString, QString> installedModFiles(QuickMod *mod, BaseInstance *instance) const;
	QString existingModFile(QuickMod *mod, const BaseVersionPtr version) const;
	QString existingModFile(QuickMod *mod, const QString &version) const;

	bool isWebsiteTrusted(const QUrl &url) const;
	void setWebsiteTrusted(const QUrl &url, const bool trusted);

	bool haveUid(const QString &uid) const;

	QList<QuickMod *> updatedModsForInstance(std::shared_ptr<BaseInstance> instance) const;

public
slots:
	void registerMod(const QString &fileName);
	void registerMod(const QUrl &url);
	void unregisterMod(QuickMod *mod);

	void updateFiles();

private
slots:
	void touchMod(QuickMod *mod);
	void addMod(QuickMod *mod);
	void clearMods();
	void removeMod(QuickMod *mod);

	void modIconUpdated();
	void modLogoUpdated();

	void cleanup();

signals:
	void modAdded(QuickMod *mod);
	void modsListChanged();
	void error(const QString &message);

private:
	friend class QuickModFilesUpdater;
	QuickModFilesUpdater *m_updater;

	QList<QuickMod *> m_mods;

	SettingsObject *m_settings;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QuickModsList::Flags)
