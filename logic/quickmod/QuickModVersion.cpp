#include "QuickModVersion.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "logic/net/HttpMetaCache.h"
#include "QuickMod.h"
#include "modutils.h"
#include "MultiMC.h"
#include "logic/MMCJson.h"

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
	type = object.contains("type") ? MMCJson::ensureString(object.value("type"), "'type'") : "Release";
	url = MMCJson::ensureUrl(object.value("url"), "'url'");
	md5 = object.value("md5").toString();
	forgeVersionFilter = object.value("forgeCompat").toString();
	compatibleVersions.clear();
	for (auto val : MMCJson::ensureArray(object.value("mcCompat"), "'mcCompat'"))
	{
		compatibleVersions.append(MMCJson::ensureString(val));
	}
	dependencies.clear();
	recommendations.clear();
	suggestions.clear();
	breaks.clear();
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
				dependencies.insert(uid, version);
			}
			else if (type == "recommends")
			{
				recommendations.insert(uid, version);
			}
			else if (type == "suggests")
			{
				suggestions.insert(uid, version);
			}
			else if (type == "breaks")
			{
				breaks.insert(uid, version);
			}
			else if (type == "conflicts")
			{
				conflicts.insert(uid, version);
			}
			else if (type == "provides")
			{
				provides.insert(uid, version);
			}
			else
			{
				throw MMCError(QObject::tr("Unknown reference type '%1'").arg(type));
			}
		}
	}

	// download type
	{
		const QString typeString = object.value("downloadType").toString("parallel");
		if (typeString == "direct")
		{
			downloadType = Direct;
		}
		else if (typeString == "parallel")
		{
			downloadType = Parallel;
		}
		else if (typeString == "sequential")
		{
			downloadType = Sequential;
		}
		else
		{
			throw new MMCError(QObject::tr("Unknown value for \"downloadType\" field"));
		}
	}
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

QuickModVersionPtr QuickModVersion::invalid(QuickModPtr mod)
{
	return QuickModVersionPtr(new QuickModVersion(mod, false));
}

QuickModVersionList::QuickModVersionList(QuickModPtr mod, InstancePtr instance, QObject *parent)
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
	return m_mod->versions().at(i);
}
int QuickModVersionList::count() const
{
	return m_mod->versions().count();
}
