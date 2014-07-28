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

#include "QuickModVersion.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "logic/net/HttpMetaCache.h"
#include "QuickMod.h"
#include "modutils.h"
#include "MultiMC.h"
#include "logic/MMCJson.h"
#include "logic/BaseInstance.h"

static QMap<QString, QString> jsonObjectToStringStringMap(const QJsonObject &obj)
{
	QMap<QString, QString> out;
	for (auto it = obj.begin(); it != obj.end(); ++it)
	{
		out.insert(it.key(), MMCJson::ensureString(it.value()));
	}
	return out;
}

void QuickModVersion::parse(const QJsonObject &object)
{
	name_ = MMCJson::ensureString(object.value("name"), "'name'");
	type = object.contains("type") ? MMCJson::ensureString(object.value("type"), "'type'")
								   : "Release";
	sha1 = object.value("sha1").toString();
	forgeVersionFilter = object.value("forgeCompat").toString();
	liteloaderVersionFilter = object.value("liteloaderCompat").toString();
	compatibleVersions.clear();
	for (auto val : MMCJson::ensureArray(object.value("mcCompat"), "'mcCompat'"))
	{
		compatibleVersions.append(MMCJson::ensureString(val));
	}
	dependencies.clear();
	recommendations.clear();
	suggestions.clear();
	conflicts.clear();
	provides.clear();
	if (object.contains("references"))
	{
		for (auto val : MMCJson::ensureArray(object.value("references"), "'references'"))
		{
			const QJsonObject obj = MMCJson::ensureObject(val, "'reference'");
			const QString uid = MMCJson::ensureString(obj.value("uid"), "'uid'");
			const QString version = MMCJson::ensureString(obj.value("version"), "'version'");
			const QString type = MMCJson::ensureString(obj.value("type"), "'type'");
			if (type == "depends")
			{
				dependencies.insert(QuickModUid(uid), qMakePair(version, obj.value("isSoft").toBool(false)));
			}
			else if (type == "recommends")
			{
				recommendations.insert(QuickModUid(uid), version);
			}
			else if (type == "suggests")
			{
				suggestions.insert(QuickModUid(uid), version);
			}
			else if (type == "conflicts")
			{
				conflicts.insert(QuickModUid(uid), version);
			}
			else if (type == "provides")
			{
				provides.insert(QuickModUid(uid), version);
			}
			else
			{
				throw MMCError(QObject::tr("Unknown reference type '%1'").arg(type));
			}
		}
	}

	libraries.clear();
	if (object.contains("libraries"))
	{
		for (auto lib : MMCJson::ensureArray(object.value("libraries"), "'libraries'"))
		{
			const QJsonObject libObj = MMCJson::ensureObject(lib, "library");
			Library library;
			library.name = MMCJson::ensureString(libObj.value("name"), "library 'name'");
			if (libObj.contains("url"))
			{
				library.url = MMCJson::ensureUrl(libObj.value("url"), "library url");
			}
			else
			{
				library.url = QUrl("http://repo1.maven.org/maven2/");
			}
			libraries.append(library);
		}
	}

	downloads.clear();
	for (auto dlValue : MMCJson::ensureArray(object.value("urls"), "'urls'"))
	{
		const QJsonObject dlObject = dlValue.toObject();
		QuickModDownload download;
		download.url = MMCJson::ensureString(dlObject.value("url"), "'url'");
		download.priority = MMCJson::ensureInteger(dlObject.value("priority"), "'priority'", 0);
		// download type
		{
			const QString typeString = dlObject.value("downloadType").toString("parallel");
			if (typeString == "direct")
			{
				download.type = QuickModDownload::Direct;
			}
			else if (typeString == "parallel")
			{
				download.type = QuickModDownload::Parallel;
			}
			else if (typeString == "sequential")
			{
				download.type = QuickModDownload::Sequential;
			}
			else if (typeString == "encoded")
			{
				download.type = QuickModDownload::Encoded;
			}
			else if (typeString == "maven")
			{
				download.type = QuickModDownload::Maven;
			}
			else
			{
				throw new MMCError(QObject::tr("Unknown value for \"downloadType\" field"));
			}
		}
		download.hint = dlObject.value("hint").toString();
		download.group = dlObject.value("group").toString();
		downloads.append(download);
	}
	std::sort(downloads.begin(), downloads.end(), [](const QuickModDownload dl1, const QuickModDownload dl2)
	{
		return dl1.priority < dl2.priority;
	});

	// install type
	{
		const QString typeString = object.value("installType").toString("forgeMod");
		if (typeString == "forgeMod")
		{
			installType = ForgeMod;
		}
		else if (typeString == "forgeCoreMod")
		{
			installType = ForgeCoreMod;
		}
		else if (typeString == "liteloaderMod")
		{
			installType = LiteLoaderMod;
		}
		else if (typeString == "extract")
		{
			installType = Extract;
		}
		else if (typeString == "configPack")
		{
			installType = ConfigPack;
		}
		else if (typeString == "group")
		{
			installType = Group;
		}
		else
		{
			throw new MMCError(QObject::tr("Unknown value for \"installType\" field"));
		}
	}
}
QJsonObject QuickModVersion::toJson() const
{
	QJsonArray refs;
	auto refToJson = [&refs](const QString &type, const QMap<QuickModUid, QString> &references)
	{
		for (auto it = references.constBegin(); it != references.constEnd(); ++it)
		{
			QJsonObject obj;
			obj.insert("type", type);
			obj.insert("uid", it.key().toString());
			obj.insert("version", it.value());
			refs.append(obj);
		}
	};

	QJsonObject obj;
	obj.insert("name", name_);
	obj.insert("mcCompat", QJsonArray::fromStringList(compatibleVersions));
	MMCJson::writeString(obj, "type", type);
	MMCJson::writeString(obj, "sha1", sha1);
	MMCJson::writeString(obj, "forgeCompat", forgeVersionFilter);
	MMCJson::writeString(obj, "liteloaderCompat", liteloaderVersionFilter);
	MMCJson::writeObjectList(obj, "libraries", libraries);
	for (auto it = dependencies.constBegin(); it != dependencies.constEnd(); ++it)
	{
		QJsonObject obj;
		obj.insert("type", type);
		obj.insert("uid", it.key().toString());
		obj.insert("version", it.value().first);
		obj.insert("isSoft", it.value().second);
		refs.append(obj);
	}
	refToJson("recommends", recommendations);
	refToJson("suggests", suggestions);
	refToJson("conflicts", conflicts);
	refToJson("provides", provides);
	obj.insert("references", refs);
	switch (installType)
	{
	case ForgeMod: obj.insert("installType", QStringLiteral("forgeMod"));
	case ForgeCoreMod: obj.insert("installType", QStringLiteral("forgeCoreMod"));
	case LiteLoaderMod: obj.insert("installType", QStringLiteral("liteloaderMod"));
	case Extract: obj.insert("installType", QStringLiteral("extract"));
	case ConfigPack: obj.insert("installType", QStringLiteral("configPack"));
	case Group: obj.insert("installType", QStringLiteral("group"));
	}
	MMCJson::writeObjectList(obj, "urls", downloads);
	return obj;
}

QuickModVersionPtr QuickModVersion::invalid(QuickModPtr mod)
{
	return QuickModVersionPtr(new QuickModVersion(mod, false));
}

QuickModVersionList::QuickModVersionList(QuickModUid mod, InstancePtr instance, QObject *parent)
	: BaseVersionList(parent), m_mod(mod), m_instance(instance)
{
}

Task *QuickModVersionList::getLoadTask()
{
	return 0;
}
bool QuickModVersionList::isLoaded()
{
	return true;
}

const BaseVersionPtr QuickModVersionList::at(int i) const
{
	return versions().at(i);
}
int QuickModVersionList::count() const
{
	return versions().count();
}

QList<QuickModVersionPtr> QuickModVersionList::versions() const
{
	// TODO repository priority
	QList<QuickModVersionPtr> out;
	for (auto mod : m_mod.mods())
	{
		for (auto version : mod->versions())
		{
			if (version->compatibleVersions.contains(m_instance->intendedVersionId()))
			{
				out.append(version);
			}
		}
	}
	return out;
}

QJsonObject QuickModVersion::Library::toJson() const
{
	QJsonObject obj;
	obj.insert("name", name);
	if (!url.isEmpty())
	{
		obj.insert("url", url.toString(QUrl::FullyEncoded));
	}
	return obj;
}
QJsonObject QuickModDownload::toJson() const
{
	QJsonObject obj;
	obj.insert("url", url);
	obj.insert("priority", priority);
	MMCJson::writeString(obj, "hint", hint);
	MMCJson::writeString(obj, "group", group);
	switch (type)
	{
	case Direct: obj.insert("downloadType", QStringLiteral("direct"));
	case Parallel: obj.insert("downloadType", QStringLiteral("parallel"));
	case Sequential: obj.insert("downloadType", QStringLiteral("sequential"));
	case Encoded: obj.insert("downloadType", QStringLiteral("encoded"));
	case Maven: obj.insert("downloadType", QStringLiteral("maven"));
	}
	return obj;
}
