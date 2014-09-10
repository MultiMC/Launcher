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
#include "QuickModMetadata.h"
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
	name_ = MMCJson::ensureString(object.value("name"), "name");
	if (object.contains("type"))
	{
		type = MMCJson::ensureString(object.value("type"), "type");
	}
	else
	{
		type = "Release";
	}
	versionString =
		object.contains("version") ? MMCJson::ensureString(object.value("version")) : name_;
	m_version = Util::Version(versionString);
	sha1 = object.value(QStringLiteral("sha1")).toString();
	forgeVersionFilter = object.value("forgeCompat").toString();
	liteloaderVersionFilter = object.value("liteloaderCompat").toString();
	compatibleVersions.clear();
	for (auto val : MMCJson::ensureArray(object.value("mcCompat"), "mcCompat"))
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
		for (auto val : MMCJson::ensureArray(object.value("references"), "references"))
		{
			const QJsonObject obj = MMCJson::ensureObject(val, "reference");
			const QString uid = MMCJson::ensureString(obj.value("uid"), "uid");
			const QuickModVersionRef version = QuickModVersionRef(
				QuickModRef(uid), MMCJson::ensureString(obj.value("version"), "version"));
			const QString type = MMCJson::ensureString(obj.value("type"), "type");
			if (type == "depends")
			{
				dependencies.insert(QuickModRef(uid),
									qMakePair(version, obj.value("isSoft").toBool(false)));
			}
			else if (type == "recommends")
			{
				recommendations.insert(QuickModRef(uid), version);
			}
			else if (type == "suggests")
			{
				suggestions.insert(QuickModRef(uid), version);
			}
			else if (type == "conflicts")
			{
				conflicts.insert(QuickModRef(uid), version);
			}
			else if (type == "provides")
			{
				provides.insert(QuickModRef(uid), version);
			}
			else
			{
				throw MMCError(QObject::tr("Unknown reference type '%1'").arg(type));
			}
		}
	}

	/*
	 * FIXME: redo. we already have a library list parser. use it here.
	 */
	/*
	libraries.clear();
	if (object.contains("libraries"))
	{
		for (auto lib : MMCJson::ensureArray(object.value("libraries"), "libraries"))
		{
			const QJsonObject libObj = MMCJson::ensureObject(lib, "library");
			Library library;
			library.name = MMCJson::ensureString(libObj.value("name"), "library 'name'");
			if (libObj.contains("url"))
			{
				library.repo = MMCJson::ensureUrl(libObj.value("repo"), "library repo");
			}
			else
			{
				library.repo = QUrl("http://repo1.maven.org/maven2/");
			}
			libraries.append(library);
		}
	}
	*/

	downloads.clear();
	for (auto dlValue : MMCJson::ensureArray(object.value("urls"), "urls"))
	{
		const QJsonObject dlObject = dlValue.toObject();
		QuickModDownload download;
		download.url = MMCJson::ensureString(dlObject.value("url"), "url");
		download.priority = MMCJson::ensureInteger(dlObject.value("priority"), "priority", 0);
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
			else
			{
				throw MMCError(QObject::tr("Unknown value for \"downloadType\" field"));
			}
		}
		download.hint = dlObject.value("hint").toString();
		download.group = dlObject.value("group").toString();
		downloads.append(download);
	}
	std::sort(downloads.begin(), downloads.end(),
			  [](const QuickModDownload dl1, const QuickModDownload dl2)
	{ return dl1.priority < dl2.priority; });

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
			throw MMCError(QObject::tr("Unknown value for \"installType\" field"));
		}
	}
}

QJsonObject QuickModVersion::toJson() const
{
	QJsonArray refs;
	auto refToJson = [&refs](const QString & type,
							 const QMap<QuickModRef, QuickModVersionRef> & references)
	{
		for (auto it = references.constBegin(); it != references.constEnd(); ++it)
		{
			QJsonObject obj;
			obj.insert("type", type);
			obj.insert("uid", it.key().toString());
			obj.insert("version", it.value().toString());
			refs.append(obj);
		}
	};

	QJsonObject obj;
	obj.insert("name", name_);
	obj.insert("mcCompat", QJsonArray::fromStringList(compatibleVersions));
	MMCJson::writeString(obj, "version", versionString);
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
		obj.insert("version", it.value().first.toString());
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
	case ForgeMod:
		obj.insert("installType", QStringLiteral("forgeMod"));
	case ForgeCoreMod:
		obj.insert("installType", QStringLiteral("forgeCoreMod"));
	case LiteLoaderMod:
		obj.insert("installType", QStringLiteral("liteloaderMod"));
	case Extract:
		obj.insert("installType", QStringLiteral("extract"));
	case ConfigPack:
		obj.insert("installType", QStringLiteral("configPack"));
	case Group:
		obj.insert("installType", QStringLiteral("group"));
	}
	MMCJson::writeObjectList(obj, "urls", downloads);
	return obj;
}

bool QuickModVersion::needsDeploy() const
{
	return installType == ForgeCoreMod;
}

QJsonObject QuickModVersion::Library::toJson() const
{
	QJsonObject obj;
	obj.insert("name", name);
	if (!repo.isEmpty())
	{
		obj.insert("url", repo.toString(QUrl::FullyEncoded));
	}
	return obj;
}
