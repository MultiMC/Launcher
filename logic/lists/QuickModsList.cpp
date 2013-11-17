#include "QuickModsList.h"

#include <QMimeData>
#include <QIcon>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "logic/QuickModFilesUpdater.h"
#include "depends/groupview/include/categorizedsortfilterproxymodel.h"

QuickMod::QuickMod(QObject *parent) : QObject(parent)
{
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
	m_name = mod.value("name").toString();
	m_description = mod.value("description").toString();
	m_nemName = mod.value("nemName").toString();
	m_modId = mod.value("modId").toString();
	m_websiteUrl = QUrl(mod.value("websiteUrl").toString());
	m_iconUrl = QUrl(mod.value("iconUrl").toString());
	m_updateUrl = QUrl(mod.value("updateUrl").toString());
	m_recommendedUrls.clear();
	foreach(const QJsonValue& val, mod.value("recommends").toArray())
	{
		m_recommendedUrls.append(QUrl(val.toString()));
	}
	m_dependentUrls.clear();
	foreach(const QJsonValue& val, mod.value("depends").toArray())
	{
		m_dependentUrls.append(QUrl(val.toString()));
	}
	m_categories.clear();
	foreach(const QJsonValue& val, mod.value("categories").toArray())
	{
		m_categories.append(val.toString());
	}
	m_tags.clear();
	foreach(const QJsonValue& val, mod.value("tags").toArray())
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
	foreach(const QJsonValue& val, mod.value("versions").toArray())
	{
		JSON_ASSERT(val.isObject());
		QJsonObject ver = val.toObject();
		Version version;
		version.name = ver.value("name").toString();
		version.url = QUrl(ver.value("url").toString());
		version.compatibleVersions.clear();
		foreach(const QJsonValue & val, mod.value("mcCompatibility").toArray())
		{
			version.compatibleVersions.append(val.toString());
		}
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

QuickModsList::QuickModsList(QObject *parent)
	: QAbstractListModel(parent), m_updater(new QuickModFilesUpdater(this))
{
	connect(this, &QuickModsList::registerModFile, m_updater, &QuickModFilesUpdater::registerFile);
	connect(this, &QuickModsList::updateModFiles, m_updater, &QuickModFilesUpdater::update);
	connect(m_updater, &QuickModFilesUpdater::addedMod, this, &QuickModsList::addMod);
	connect(m_updater, &QuickModFilesUpdater::clearMods, this, &QuickModsList::clearMods);
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
	return m_mods.size();
}
Qt::ItemFlags QuickModsList::flags(const QModelIndex &index) const
{
	return Qt::ItemIsDropEnabled | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant QuickModsList::data(const QModelIndex &index, int role) const
{
	if (0 <= index.row() || index.row() < m_mods.size())
	{
		return QVariant();
	}

	QuickMod *mod = m_mods.at(index.row());

	switch (role)
	{
	case Qt::DisplayRole:
		return mod->name();
	case Qt::DecorationRole:
		return QIcon(); // TODO we've got an url, now what?
	case Qt::ToolTipRole:
		return mod->description();
	case NameRole:
		return mod->name();
	case DescriptionRole:
		return mod->description();
	case WebsiteRole:
		return mod->websiteUrl();
	case IconRole:
		return QIcon(); // TODO we've got an url, now what?
	case LogoRole:
		return QIcon(); // TODO we've got an url, now what?
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
	case KCategorizedSortFilterProxyModel::CategoryDisplayRole:
	case KCategorizedSortFilterProxyModel::CategorySortRole:
		return mod->categories().first(); // the first category is seen as the "primary"
										  // category
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

void QuickModsList::registerMod(const QString &fileName)
{
	registerMod(QUrl::fromLocalFile(fileName));
}
void QuickModsList::registerMod(const QUrl &url)
{
	emit registerModFile(url);
}

void QuickModsList::updateFiles()
{
	emit updateModFiles();
}

void QuickModsList::addMod(QuickMod *mod)
{
	for (int i = 0; i < m_mods.size(); ++i)
	{
		if (m_mods.at(i)->compare(mod))
		{
			m_mods.replace(i, mod);
			return;
		}
	}

	beginInsertRows(QModelIndex(), rowCount(QModelIndex()), rowCount(QModelIndex()) + 1);
	m_mods.append(mod);
	endInsertRows();
}

void QuickModsList::clearMods()
{
	beginResetModel();
	qDeleteAll(m_mods);
	m_mods.clear();
	endResetModel();
}

void QuickModsList::removeMod(QuickMod *mod)
{
	if (!m_mods.contains(mod))
	{
		return;
	}

	beginRemoveRows(QModelIndex(), m_mods.indexOf(mod), m_mods.indexOf(mod));
	m_mods.takeAt(m_mods.indexOf(mod));
	endRemoveRows();
}
