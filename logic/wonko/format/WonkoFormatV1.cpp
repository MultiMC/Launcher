#include "WonkoFormatV1.h"

#include "Json.h"

#include "wonko/WonkoIndex.h"
#include "wonko/WonkoVersion.h"
#include "wonko/WonkoVersionList.h"

using namespace Json;

static WonkoVersionPtr parseCommonVersion(const QString &uid, const QJsonObject &obj)
{
	const QVector<QJsonObject> requiresRaw = obj.contains("requires") ? requireIsArrayOf<QJsonObject>(obj, "requires") : QVector<QJsonObject>();
	QVector<WonkoReference> requires;
	requires.reserve(requiresRaw.size());
	std::transform(requiresRaw.begin(), requiresRaw.end(), std::back_inserter(requires), [](const QJsonObject &rObj)
	{
		WonkoReference ref(requireString(rObj, "uid"));
		ref.setVersion(ensureString(rObj, "version", QString()));
		return ref;
	});

	WonkoVersionPtr version = std::make_shared<WonkoVersion>(uid, requireString(obj, "version"));
	if (obj.value("time").isString())
	{
		version->setTime(QDateTime::fromString(requireString(obj, "time"), Qt::ISODate).toMSecsSinceEpoch() / 1000);
	}
	else
	{
		version->setTime(requireInteger(obj, "time"));
	}
	version->setType(ensureString(obj, "type", QString()));
	version->setRequires(requires);
	return version;
}
static QJsonObject serializeCommonVersion(const WonkoVersion *version)
{
	QJsonArray requires;
	for (const WonkoReference &ref : version->requires())
	{
		if (ref.version().isEmpty())
		{
			requires.append(QJsonObject({{"uid", ref.uid()}}));
		}
		else
		{
			requires.append(QJsonObject({
											{"uid", ref.uid()},
											{"version", ref.version()}
										}));
		}
	}

	const QJsonObject obj = {
		{"version", version->version()},
		{"type", version->type()},
		{"time", version->time().toString(Qt::ISODate)},
		{"requires", requires}
	};
	return obj;
}

BaseWonkoEntity::Ptr WonkoFormatV1::parseIndexInternal(const QJsonObject &obj) const
{
	const QVector<QJsonObject> objects = requireIsArrayOf<QJsonObject>(obj, "index");
	QVector<WonkoVersionListPtr> lists;
	lists.reserve(objects.size());
	std::transform(objects.begin(), objects.end(), std::back_inserter(lists), [](const QJsonObject &obj)
	{
		WonkoVersionListPtr list = std::make_shared<WonkoVersionList>(requireString(obj, "uid"));
		list->setName(ensureString(obj, "name", QString()));
		return list;
	});
	return std::make_shared<WonkoIndex>(lists);
}
BaseWonkoEntity::Ptr WonkoFormatV1::parseVersionInternal(const QJsonObject &obj) const
{
	WonkoVersionPtr version = parseCommonVersion(requireString(obj, "uid"), obj);
	version->setData(
				ensureString(obj, "mainClass"),
				ensureString(obj, "appletClass"),
				ensureString(obj, "assets"),
				obj.value("minecraftArguments").isString() ?
					ensureString(obj, "minecraftArguments")
				  : QStringList(ensureIsArrayOf<QString>(obj, "minecraftArguments").toList()).join(' '),
				ensureIsArrayOf<QString>(obj, "+tweakers").toList(),
				ensureIsArrayOf<QJsonObject>(obj, "+libraries"),
				ensureIsArrayOf<QJsonObject>(obj, "+jarMods"),
				ensureInteger(obj, "order", 99)
				);
	return version;
}
BaseWonkoEntity::Ptr WonkoFormatV1::parseVersionListInternal(const QJsonObject &obj) const
{
	const QString uid = requireString(obj, "uid");

	const QVector<QJsonObject> versionsRaw = requireIsArrayOf<QJsonObject>(obj, "versions");
	QVector<WonkoVersionPtr> versions;
	versions.reserve(versionsRaw.size());
	std::transform(versionsRaw.begin(), versionsRaw.end(), std::back_inserter(versions), [this, uid](const QJsonObject &vObj)
	{ return parseCommonVersion(uid, vObj); });

	WonkoVersionListPtr list = std::make_shared<WonkoVersionList>(uid);
	list->setName(ensureString(obj, "name", QString()));
	list->setVersions(versions);
	return list;
}

QJsonObject WonkoFormatV1::serializeIndexInternal(const WonkoIndex *ptr) const
{
	QJsonArray index;
	for (const WonkoVersionListPtr &list : ptr->lists())
	{
		index.append(QJsonObject({
									 {"uid", list->uid()},
									 {"name", list->name()}
								 }));
	}
	return QJsonObject({
						   {"formatVersion", 1},
						   {"index", index}
					   });
}
QJsonObject WonkoFormatV1::serializeVersionInternal(const WonkoVersion *ptr) const
{
	QJsonObject obj = serializeCommonVersion(ptr);
	obj.insert("formatVersion", 1);
	obj.insert("uid", ptr->uid());
	auto insertUnlessNull = [&obj](const QString &key, const QString &value)
	{
		if (!value.isNull())
		{
			obj.insert(key, value);
		}
	};
	auto vectorToJsArray = [](const QVector<QJsonObject> &vector)
	{
		QJsonArray array;
		for (const QJsonObject &o : vector)
		{
			array.append(o);
		}
		return array;
	};

	insertUnlessNull("mainClass", ptr->mainClass());
	insertUnlessNull("appletClass", ptr->appletClass());
	insertUnlessNull("assets", ptr->assets());
	insertUnlessNull("minecraftArguments", ptr->minecraftArguments());
	obj.insert("+tweakers", QJsonArray::fromStringList(ptr->tweakers()));
	obj.insert("+libraries", vectorToJsArray(ptr->libraries()));
	obj.insert("+jarMods", vectorToJsArray(ptr->jarMods()));
	obj.insert("order", ptr->order());

	return obj;
}
QJsonObject WonkoFormatV1::serializeVersionListInternal(const WonkoVersionList *ptr) const
{
	QJsonArray versions;
	for (const WonkoVersionPtr &version : ptr->versions())
	{
		versions.append(serializeCommonVersion(version.get()));
	}
	return QJsonObject({
						   {"formatVersion", 10},
						   {"uid", ptr->uid()},
						   {"name", ptr->name().isNull() ? QJsonValue() : ptr->name()},
						   {"versions", versions}
					   });
}
