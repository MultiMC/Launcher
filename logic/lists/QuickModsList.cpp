#include "QuickModsList.h"

#include <QMimeData>
#include <QIcon>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "logic/net/CacheDownload.h"
#include "logic/net/NetJob.h"
#include "logic/QuickModFilesUpdater.h"
#include "logic/Mod.h"
#include "logic/BaseInstance.h"
#include "depends/settings/include/setting.h"
#include "MultiMC.h"
#include "depends/groupview/include/categorizedsortfilterproxymodel.h"
#include "logic/lists/InstanceList.h"

#include "depends/settings/include/inisettingsobject.h"

// TODO test
// TODO updating of mods
// TODO fixing markModAs*

QuickMod::QuickMod(QObject *parent) : QObject(parent), m_stub(false), m_noFetchImages(false)
{
}

QIcon QuickMod::icon()
{
	fetchImages();
	return m_icon;
}
QPixmap QuickMod::logo()
{
	fetchImages();
	return m_logo;
}

#define MALFORMED_JSON_X(message)                                                              \
	if (errorMessage)                                                                          \
	{                                                                                          \
		*errorMessage = message;                                                               \
	}                                                                                          \
	return false
#define MALFORMED_JSON MALFORMED_JSON_X(tr("Malformed QuickMod file"))
#define JSON_ASSERT_X(assertation, message)                                                    \
	if (!(assertation))                                                                        \
	{                                                                                          \
		MALFORMED_JSON_X(message);                                                             \
	}                                                                                          \
	qt_noop()
#define JSON_ASSERT(assertation)                                                               \
	if (!(assertation))                                                                        \
	{                                                                                          \
		MALFORMED_JSON;                                                                        \
	}                                                                                          \
	qt_noop()
bool QuickMod::parse(const QByteArray &data, QString *errorMessage)
{
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(data, &error);
	JSON_ASSERT_X(error.error == QJsonParseError::NoError, error.errorString());
	JSON_ASSERT(doc.isObject());
	QJsonObject mod = doc.object();
	m_stub = mod.value("stub").toBool(false);
	m_name = mod.value("name").toString();
	m_description = mod.value("description").toString();
	m_nemName = mod.value("nemName").toString();
	m_modId = mod.value("modId").toString();
	m_websiteUrl = QUrl(mod.value("websiteUrl").toString());
	m_iconUrl = QUrl(mod.value("iconUrl").toString());
	m_logoUrl = QUrl(mod.value("logoUrl").toString());
	m_updateUrl = QUrl(mod.value("updateUrl").toString());
	m_recommendedUrls.clear();
	foreach (const QJsonValue& val, mod.value("recommends").toArray())
	{
		m_recommendedUrls.append(QUrl(val.toString()));
	}
	m_dependentUrls.clear();
	foreach (const QJsonValue& val, mod.value("depends").toArray())
	{
		m_dependentUrls.append(QUrl(val.toString()));
	}
	m_categories.clear();
	foreach (const QJsonValue& val, mod.value("categories").toArray())
	{
		m_categories.append(val.toString());
	}
	m_tags.clear();
	foreach (const QJsonValue& val, mod.value("tags").toArray())
	{
		m_tags.append(val.toString());
	}
	const QString modType = mod.value("type").toString();
	if (modType == "forgeMod")
	{
		m_type = ForgeMod;
	}
	else if (modType == "forgeCoreMod")
	{
		m_type = ForgeCoreMod;
	}
	else if (modType == "resourcePack")
	{
		m_type = ResourcePack;
	}
	else if (modType == "configPack")
	{
		m_type = ConfigPack;
	}
	else
	{
		MALFORMED_JSON_X(tr("Unknown version type: %1").arg(modType));
	}
	m_versions.clear();
	foreach (const QJsonValue& val, mod.value("versions").toArray())
	{
		JSON_ASSERT(val.isObject());
		QJsonObject ver = val.toObject();
		Version version;
		version.name = ver.value("name").toString();
		version.url = QUrl(ver.value("url").toString());
		version.compatibleVersions.clear();
		foreach (const QJsonValue &val, ver.value("mcCompatibility").toArray())
		{
			version.compatibleVersions.append(val.toString());
		}
		version.dependencies.clear();
		QJsonObject deps = ver.value("dependencies").toObject();
		foreach (const QString &mod, deps.keys())
		{
			version.dependencies.insert(mod, deps.value(mod).toString());
		}
		m_versions.append(version);
	}

	if (!m_noFetchImages)
	{
		fetchImages();
	}
}
#undef MALFORMED_JSON_X
#undef MALFORMED_JSON
#undef JSON_ASSERT_X
#undef JSON_ASSERT

bool QuickMod::compare(const QuickMod *other) const
{
	return m_name == other->name();
}

void QuickMod::iconDownloadFinished(int index)
{
	auto download = qobject_cast<CacheDownload*>(sender());
	m_icon = QIcon(download->m_target_path);
	emit iconUpdated();
}
void QuickMod::logoDownloadFinished(int index)
{
	auto download = qobject_cast<CacheDownload*>(sender());
	m_logo = QPixmap(download->m_target_path);
	emit logoUpdated();
}

void QuickMod::fetchImages()
{
	auto job = new NetJob("QuickMod image download: " + m_name);
	if (m_iconUrl.isValid() && m_icon.isNull())
	{
		auto icon = CacheDownload::make(m_iconUrl, MMC->metacache()->resolveEntry("quickmod/icons", fileName(m_iconUrl)));
		connect(icon.get(), &CacheDownload::succeeded, this, &QuickMod::iconDownloadFinished);
		job->addNetAction(icon);
	}
	if (m_logoUrl.isValid() && m_logo.isNull())
	{
		auto logo = CacheDownload::make(m_logoUrl, MMC->metacache()->resolveEntry("quickmod/logos", fileName(m_logoUrl)));
		connect(logo.get(), &CacheDownload::succeeded, this, &QuickMod::logoDownloadFinished);
		job->addNetAction(logo);
	}
	job->start();
}

QString QuickMod::fileName(const QUrl &url) const
{
	const QString path = url.path();
	// TODO what if we don't have a mod id? (non-forge mod)
	return m_modId + path.mid(path.lastIndexOf("."));
}

QuickModsList::QuickModsList(QObject *parent)
	: QAbstractListModel(parent), m_updater(new QuickModFilesUpdater(this)), m_settings(new INISettingsObject("quickmod.cfg", this))
{
	m_settings->registerSetting(new Setting("AvailableMods", QVariant::fromValue(QMap<QString, QMap<QString, QString> >())));
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
	roles[RecommendedUrlsRole] = "recommendedUrls";
	roles[DependentUrlsRole] = "dependentUrls";
	roles[NemNameRole] = "nemName";
	roles[ModIdRole] = "modId";
	roles[CategoriesRole] = "categories";
	roles[TagsRole] = "tags";
	roles[TypeRole] = "type";
	roles[VersionsRole] = "versions";
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
		return mod->icon();
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
	case RecommendedUrlsRole:
		return QVariant::fromValue(mod->recommendedUrls());
	case DependentUrlsRole:
		return QVariant::fromValue(mod->dependentUrls());
	case NemNameRole:
		return mod->nemName();
	case ModIdRole:
		return mod->modId();
	case CategoriesRole:
		return mod->categories();
	case TagsRole:
		return mod->tags();
	case TypeRole:
		return mod->type();
	case VersionsRole:
		return QVariant::fromValue(mod->versions());
	case QuickModRole:
		return QVariant::fromValue(mod);
	case IsStubRole:
		return mod->isStub();
	case KCategorizedSortFilterProxyModel::CategoryDisplayRole:
	case KCategorizedSortFilterProxyModel::CategorySortRole:
		return mod->categories().isEmpty() ? "" : mod->categories().first(); // the first category is seen as the "primary" category
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

void QuickModsList::modAddedBy(const Mod &mod, BaseInstance *instance)
{
	QuickMod *qMod = m_updater->ensureExists(mod);
	markModAsInstalled(qMod, 0, mod.filename().absoluteFilePath(), instance);
}
void QuickModsList::modRemovedBy(const Mod &mod, BaseInstance *instance)
{
	QuickMod *qMod = m_updater->ensureExists(mod);
	markModAsUninstalled(qMod, 0, instance);
}

void QuickModsList::markModAsExists(QuickMod *mod, const int version, const QString &fileName)
{
	auto mods = m_settings->getSetting("AvailableMods")->get().toMap();
	auto map = mods[mod->modId()].toMap();
	map[mod->version(version).name] = fileName;
	mods[mod->modId()] = map;
	m_settings->getSetting("AvailableMods")->set(QVariant(mods));
}

void QuickModsList::markModAsInstalled(QuickMod *mod, const int version, const QString &fileName, BaseInstance *instance)
{
	auto mods = instance->settings().getSetting("InstalledMods")->get().toMap();
	auto map = mods[mod->modId()].toMap();
	map[mod->version(version).name] = fileName;
	mods[mod->modId()] = map;
	instance->settings().getSetting("InstalledMods")->set(QVariant(mods));
}
void QuickModsList::markModAsUninstalled(QuickMod *mod, const int version, BaseInstance *instance)
{
	auto mods = instance->settings().getSetting("InstalledMods")->get().toMap();
	auto map = mods[mod->modId()].toMap();
	map.remove(mod->version(version).name);
	if (map.isEmpty())
	{
		mods.remove(mod->modId());
	}
	else
	{
		mods[mod->modId()] = map;
	}
	instance->settings().set("InstalledMods", QVariant(mods));
}
bool QuickModsList::isModMarkedAsInstalled(QuickMod *mod, const int version, BaseInstance *instance) const
{
	auto mods = instance->settings().getSetting("InstalledMods")->get().toMap();
	return mods.contains(mod->modId()) && mods.value(mod->modId()).toMap().contains(mod->version(version).name);
}
bool QuickModsList::isModMarkedAsExists(QuickMod *mod, const int version) const
{
	auto mods = m_settings->getSetting("AvailableMods")->get().toMap();
	return mods.contains(mod->modId()) && mods.value(mod->modId()).toMap().contains(mod->version(version).name);
}
QString QuickModsList::installedModFile(QuickMod *mod, const int version, BaseInstance *instance) const
{
	if (!isModMarkedAsInstalled(mod, version, instance))
	{
		return QString();
	}
	auto mods = instance->settings().getSetting("InstalledMods")->get().toMap();
	return mods[mod->modId()].toMap()[mod->version(version).name].toString();
}
QString QuickModsList::existingModFile(QuickMod *mod, const int version) const
{
	if (!isModMarkedAsExists(mod, version))
	{
		return QString();
	}
	auto mods = m_settings->getSetting("AvailableMods")->get().toMap();
	return mods[mod->modId()].toMap()[mod->version(version).name].toString();
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
			disconnect(m_mods.at(i), &QuickMod::iconUpdated, this, &QuickModsList::modIconUpdated);
			disconnect(m_mods.at(i), &QuickMod::logoUpdated, this, &QuickModsList::modLogoUpdated);
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
	auto mod = qobject_cast<QuickMod*>(sender());
	auto modIndex = index(m_mods.indexOf(mod), 0);
	emit dataChanged(modIndex, modIndex, QVector<int>() << Qt::DecorationRole << IconRole);
}
void QuickModsList::modLogoUpdated()
{
	auto mod = qobject_cast<QuickMod*>(sender());
	auto modIndex = index(m_mods.indexOf(mod), 0);
	emit dataChanged(modIndex, modIndex, QVector<int>() << LogoRole);
}
