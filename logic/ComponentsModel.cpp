#include "ComponentsModel.h"

#include <QMimeData>
#include <QDir>

#include "logic/InstanceList.h"
#include "logic/MMCJson.h"
#include "logic/OneSixInstance.h"
#include "logic/LegacyInstance.h"
#include "logic/FileHashCache.h"
#include "MultiMC.h"

ComponentsModel::ComponentsModel(QObject *parent) : QAbstractListModel(parent)
{
	update();
}

int ComponentsModel::rowCount(const QModelIndex &parent) const
{
	return m_components.size();
}
QVariant ComponentsModel::data(const QModelIndex &index, int role) const
{
	Component comp = m_components[index.row()];
	if (role == Qt::DisplayRole)
	{
		return comp.name + ' ' + comp.version;
	}
	else
	{
		return QVariant();
	}
}

Qt::ItemFlags ComponentsModel::flags(const QModelIndex &index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable;
}

QStringList ComponentsModel::mimeTypes() const
{
	return QStringList() << "application/x-multimc-components";
}
QMimeData *ComponentsModel::mimeData(const QModelIndexList &indexes) const
{
	QJsonArray array;
	for (const auto index : indexes)
	{
		array.append(m_components[index.row()].toJson());
	}
	QMimeData *data = new QMimeData;
	data->setData("application/x-multimc-components", QJsonDocument(array).toJson());
	return data;
}
bool ComponentsModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row,
									  int column, const QModelIndex &parent) const
{
	// we don't allow any drops, but for some reason we can't return IgnoreAction from
	// supportedDropActions, so we need to prevent dropping on self
	return false;
}
Qt::DropActions ComponentsModel::supportedDragActions() const
{
	return Qt::CopyAction;
}
Qt::DropActions ComponentsModel::supportedDropActions() const
{
	return Qt::CopyAction;
}

void ComponentsModel::update()
{
	QList<QByteArray> hashes; // duplicate prevention
	auto componentsFor =
		[&hashes](InstancePtr instance, Component::Type type, std::shared_ptr<ModList> list)
	{
		QList<Component> comps;
		for (int i = 0; i < list->size(); ++i)
		{
			Mod mod = list->operator[](i);
			const QByteArray hash = FileHashCache::instance()->hash(mod.filename());
			if (mod.type() == Mod::MOD_FOLDER || hashes.contains(hash))
			{
				continue;
			}
			hashes.append(hash);
			Component comp;
			comp.type = type;
			comp.name = mod.name();
			comp.version = mod.version();
			comp.path = mod.filename().absoluteFilePath();
			comp.mcversion =
				mod.mcversion().isEmpty() ? instance->currentVersionId() : mod.mcversion();
			comp.origin = instance;
			comps.append(comp);
		}
		return comps;
	};

	QList<Component> components;
	for (int i = 0; i < MMC->instances()->rowCount(); ++i)
	{
		InstancePtr instance = MMC->instances()->at(i);
		components +=
			componentsFor(instance, Component::ResourcePack, instance->resourcePackList());
		components +=
			componentsFor(instance, Component::TexturePack, instance->texturePackList());
		components += componentsFor(instance, Component::LoaderMod, instance->loaderModList());
		components += componentsFor(instance, Component::CoreMod, instance->coreModList());
	}

	beginResetModel();
	m_components = components;
	endResetModel();
}

QString ComponentsModel::Component::typeAsString() const
{
	switch (type)
	{
	case LoaderMod:
		return "loadermod";
	case CoreMod:
		return "coremod";
	case ResourcePack:
		return "resourcepack";
	case TexturePack:
		return "texturepack";
	}
	return QString();
}

QJsonObject ComponentsModel::Component::toJson() const
{
	QJsonObject obj;
	obj.insert("name", name);
	obj.insert("version", version);
	obj.insert("path", path);
	obj.insert("mcversion", mcversion);
	obj.insert("origin", origin->id());
	obj.insert("type", typeAsString());
	return obj;
}
ComponentsModel::Component ComponentsModel::Component::fromJson(const QJsonObject &object)
{
	Component out;
	out.name = MMCJson::ensureString(object.value("name"));
	out.version = MMCJson::ensureString(object.value("version"));
	out.path = MMCJson::ensureString(object.value("path"));
	out.mcversion = MMCJson::ensureString(object.value("mcversion"));
	out.origin =
		MMC->instances()->getInstanceById(MMCJson::ensureString(object.value("origin")));
	const QString type = MMCJson::ensureString(object.value("type"));
	if (type == "loadermod")
	{
		out.type = LoaderMod;
	}
	else if (type == "coremod")
	{
		out.type = CoreMod;
	}
	else if (type == "resourcepack")
	{
		out.type = ResourcePack;
	}
	else if (type == "texturepack")
	{
		out.type = TexturePack;
	}
	return out;
}
QList<ComponentsModel::Component> ComponentsModel::Component::fromJson(const QByteArray &data)
{
	QList<Component> out;
	for (const auto val : MMCJson::ensureArray(MMCJson::parseDocument(data, "component JSON")))
	{
		out.append(fromJson(MMCJson::ensureObject(val)));
	}
	return out;
}
