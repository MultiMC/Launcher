#pragma once

#include <QAbstractListModel>
#include <QUrl>
#include <QStringList>
#include <QIcon>
#include <QPixmap>
#include <QCryptographicHash>

class QuickModFilesUpdater;
class Mod;
class BaseInstance;
class SettingsObject;

class QuickMod : public QObject
{
	Q_OBJECT
public:
	explicit QuickMod(QObject *parent = 0);

	enum ModType
	{
		ForgeMod,
		ForgeCoreMod,
		ResourcePack,
		ConfigPack
	};
	Q_ENUMS(ModType)

	struct Version
	{
		Version() {}
		Version(const QString &name,
				const QUrl &url,
				const QStringList &mc,
				const QString &forge = QString(),
				const QMap<QString, QString> &deps = QMap<QString, QString>(),
				const QMap<QString, QString> &recs = QMap<QString, QString>(),
				const QByteArray &checksum = QByteArray()) :
			name(name), url(url), compatibleVersions(mc), forgeVersionFilter(forge), dependencies(deps), recommendations(recs), checksum(checksum) {}
		QString name;
		QUrl url;
		QStringList compatibleVersions;
		// TODO actually make use of the forge version provided
		QString forgeVersionFilter;
		QMap<QString, QString> dependencies;
		QMap<QString, QString> recommendations;
		QByteArray checksum;
		QCryptographicHash::Algorithm checksum_algorithm;

		bool operator==(const Version &other) const
		{
			return name == other.name &&
					url == other.url &&
					compatibleVersions == other.compatibleVersions &&
					forgeVersionFilter == other.forgeVersionFilter &&
					dependencies == other.dependencies &&
					recommendations == other.recommendations &&
					checksum == other.checksum;
		}
	};

	bool isValid() const
	{
		return !m_name.isEmpty();
	}

	QString name() const { return m_name; }
	QString description() const { return m_description; }
	QUrl websiteUrl() const { return m_websiteUrl; }
	QUrl iconUrl() const { return m_iconUrl; }
	QIcon icon();
	QUrl logoUrl() const { return m_logoUrl; }
	QPixmap logo();
	QUrl updateUrl() const { return m_updateUrl; }
	QMap<QString, QUrl> references() const { return m_references; }
	QString nemName() const { return m_nemName; }
	QString modId() const { return m_modId; }
	QStringList categories() const { return m_categories; }
	QStringList tags() const { return m_tags; }
	ModType type() const { return m_type; }
	QList<Version> versions() const { return m_versions; }
	bool isStub() const { return m_stub; }

	int numVersions() const { return m_versions.size(); }
	Version version(const int index) const { return m_versions.at(index); }

	bool parse(const QByteArray &data, QString *errorMessage = 0);

	bool compare(const QuickMod *other) const;

signals:
	void iconUpdated();
	void logoUpdated();

private
slots:
	void iconDownloadFinished(int index);
	void logoDownloadFinished(int index);

private:
	friend class QuickModFilesUpdater;
	friend class QuickModTest;
	friend class QuickModsListTest;
	friend class TestsInternal;
	QString m_name;
	QString m_description;
	QUrl m_websiteUrl;
	QUrl m_iconUrl;
	QIcon m_icon;
	QUrl m_logoUrl;
	QPixmap m_logo;
	QUrl m_updateUrl;
	QMap<QString, QUrl> m_references;
	QString m_nemName;
	QString m_modId;
	QStringList m_categories;
	QStringList m_tags;
	ModType m_type;
	QList<Version> m_versions;
	bool m_stub;

	void fetchImages();
	QString fileName(const QUrl& url) const;
};
Q_DECLARE_METATYPE(QuickMod*)
Q_DECLARE_METATYPE(QuickMod::Version)

class QuickModsList : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit QuickModsList(QObject *parent = 0);
	~QuickModsList();

	enum Roles
	{
		NameRole = Qt::UserRole,
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
		TypeRole,
		VersionsRole,
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

	int numMods() const { return m_mods.size(); }
	QuickMod *modAt(const int index) const { return m_mods[index]; }

	void modAddedBy(const Mod &mod, BaseInstance *instance);
	void modRemovedBy(const Mod &mod, BaseInstance *instance);

	void markModAsExists(QuickMod *mod, const int version, const QString &fileName);
	void markModAsInstalled(QuickMod *mod, const int version, const QString &fileName, BaseInstance *instance);
	void markModAsUninstalled(QuickMod *mod, const int version, BaseInstance *instance);
	bool isModMarkedAsInstalled(QuickMod *mod, const int version, BaseInstance *instance) const;
	bool isModMarkedAsExists(QuickMod *mod, const int version) const;
	QString installedModFile(QuickMod *mod, const int version, BaseInstance *instance) const;
	QString existingModFile(QuickMod *mod, const int version) const;

public
slots:
	void registerMod(const QString &fileName);
	void registerMod(const QUrl &url);

	void updateFiles();

private
slots:
	void addMod(QuickMod *mod);
	void clearMods();
	void removeMod(QuickMod *mod);

	void modIconUpdated();
	void modLogoUpdated();

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
