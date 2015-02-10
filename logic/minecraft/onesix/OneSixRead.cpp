#include "OneSixFormat.h"
#include <QJsonArray>

#include "minecraft/Package.h"
#include "Json.h"
#include "minecraft/Assets.h"
#include "wonko/Rules.h"

using namespace Json;

static bool parse_timestamp(const QString &raw, QString &save_here, QDateTime &parse_here)
{
	save_here = raw;
	if (save_here.isEmpty())
	{
		return false;
	}
	parse_here = QDateTime::fromString(save_here, Qt::ISODate);
	if (!parse_here.isValid())
	{
		return false;
	}
	return true;
}

RulesPtr readLibraryRules(const QJsonObject &objectWithRules)
{
	QList<std::shared_ptr<BaseRule>> rules;
	auto rulesVal = objectWithRules.value("rules");
	if (!rulesVal.isArray())
		return std::make_shared<Rules>(rules);

	QJsonArray ruleList = rulesVal.toArray();
	for (auto ruleVal : ruleList)
	{
		std::shared_ptr<BaseRule> rule;
		if (!ruleVal.isObject())
			continue;
		auto ruleObj = ruleVal.toObject();
		auto actionVal = ruleObj.value("action");
		if (!actionVal.isString())
			continue;
		auto action = BaseRule::actionFromString(actionVal.toString());
		if (action == BaseRule::Defer)
			continue;

		auto osVal = ruleObj.value("os");
		if (!osVal.isObject())
		{
			// add a new implicit action rule
			rules.append(std::make_shared<ImplicitRule>(action));
			continue;
		}

		auto osObj = osVal.toObject();
		auto osNameVal = osObj.value("name");
		if (!osNameVal.isString())
			continue;
		OpSys requiredOs = OpSys::fromString(osNameVal.toString());
		QString versionRegex = osObj.value("version").toString();
		// add a new OS rule
		rules.append(std::make_shared<OsRule>(action, requiredOs, versionRegex));
	}
	return std::make_shared<Rules>(rules);
}

LibraryPtr readRawLibrary(const QJsonObject &libObj, const QString &filename)
{
	LibraryPtr out = std::make_shared<Library>();
	if (!libObj.contains("name"))
	{
		throw JsonException(filename + " contains a library that doesn't have a 'name' field");
	}
	out->m_name = libObj.value("name").toString();

	out->m_base_url = ensureUrl(libObj, "url", QUrl());
	out->m_hint = ensureString(libObj, "MMC-hint", QString());
	out->m_absolute_url = ensureUrl(libObj, "MMC-absulute_url", QUrl());
	out->m_absolute_url = ensureUrl(libObj, "MMC-absoluteUrl", out->m_absolute_url);
	out->m_absolute_url = ensureUrl(libObj, "absoluteUrl", out->m_absolute_url);
	if (libObj.contains("extract"))
	{
		out->applyExcludes = true;
		auto extractObj = ensureObject(libObj, "extract");
		for (auto excludeVal : ensureArray(extractObj, "exclude"))
		{
			out->extract_excludes.append(ensureString(excludeVal));
		}
	}
	if (libObj.contains("natives"))
	{
		QJsonObject nativesObj = ensureObject(libObj, "natives");
		for (auto it = nativesObj.begin(); it != nativesObj.end(); ++it)
		{
			if (!it.value().isString())
			{
				qWarning() << filename << "contains an invalid native (skipping)";
			}
			OpSys opSys = OpSys::fromString(it.key());
			if (opSys != OpSys::Other)
			{
				out->m_native_classifiers[opSys] = it.value().toString();
			}
		}
	}
	if (libObj.contains("rules"))
	{
		out->applyRules = true;
		out->m_rules = std::make_shared<Rules>();
		out->m_rules->load(ensureObject(libObj, "rules"));
	}
	return out;
}

LibraryPtr OneSixFormat::readRawLibraryPlus(const QJsonObject &libObj, const QString &filename)
{
	LibraryPtr lib = readRawLibrary(libObj, filename);
	if (libObj.contains("insert"))
	{
		QJsonValue insertVal = libObj.value("insert");
		if (insertVal.isString())
		{
			// it's just a simple string rule. OK.
			QString insertString = insertVal.toString();
			if (insertString == "apply")
			{
				lib->insertType = Library::Apply;
			}
			else if (insertString == "prepend")
			{
				lib->insertType = Library::Prepend;
			}
			else if (insertString == "append")
			{
				lib->insertType = Library::Append;
			}
			else if (insertString == "replace")
			{
				lib->insertType = Library::Replace;
			}
			else
			{
				throw JsonException("A '+' library in " + filename +
									" contains an invalid insert type");
			}
		}
		else if (insertVal.isObject())
		{
			// it's a more complex rule, specifying what should be:
			//   * replaced (for now only this)
			// this was never used, AFAIK. tread carefully.
			QJsonObject insertObj = insertVal.toObject();
			if (insertObj.isEmpty())
			{
				throw JsonException("Empty compound insert rule in " + filename);
			}
			QString insertString = insertObj.keys().first();
			// really, only replace makes sense in combination with
			if (insertString != "replace")
			{
				throw JsonException("Compound insert rule is not 'replace' in " + filename);
			}
			lib->insertData = insertObj.value(insertString).toString();
		}
		else
		{
			throw JsonException("A '+' library in " + filename +
								" contains an unknown/invalid insert rule");
		}
	}
	if (libObj.contains("MMC-depend"))
	{
		const QString dependString = ensureString(libObj, "MMC-depend");
		if (dependString == "hard")
		{
			lib->dependType = Library::Hard;
		}
		else if (dependString == "soft")
		{
			lib->dependType = Library::Soft;
		}
		else
		{
			throw JsonException("A '+' library in " + filename +
								" contains an invalid depend type");
		}
	}
	return lib;
}

PackagePtr OneSixFormat::fromJson(const QJsonDocument &doc, const QString &filename,
								  const bool requireOrder)
{
	PackagePtr out(new Package());
	if (doc.isEmpty() || doc.isNull())
	{
		throw JsonException(filename + " is empty or null");
	}
	if (!doc.isObject())
	{
		throw JsonException(filename + " is not an object");
	}

	QJsonObject root = doc.object();

	if (requireOrder)
	{
		if (root.contains("order"))
		{
			out->setOrder(ensureInteger(root, "order"));
		}
		else
		{
			// FIXME: evaluate if we don't want to throw exceptions here instead
			qCritical() << filename << "doesn't contain an order field";
		}
	}

	out->name = root.value("name").toString();
	out->fileId = root.value("fileId").toString();
	out->version = root.value("version").toString();
	QString mcVersion = root.value("mcVersion").toString();
	if (!mcVersion.isEmpty())
	{
		out->dependencies["net.minecraft"] = mcVersion;
	}
	out->setPatchFilename(filename);

	auto readString = [root](const QString &key, QString &variable)
	{
		if (root.contains(key))
		{
			variable = ensureString(root, key);
		}
	};

	auto readStringRet = [root](const QString &key) -> QString
	{
		if (root.contains(key))
		{
			return ensureString(root, key);
		}
		return QString();
	};

	auto &resourceData = out->resources;
	readString("mainClass", resourceData.mainClass);
	readString("appletClass", resourceData.appletClass);
	{
		QString minecraftArguments;
		QString processArguments;
		readString("minecraftArguments", minecraftArguments);
		if (minecraftArguments.isEmpty())
		{
			readString("processArguments", processArguments);
			QString toCompare = processArguments.toLower();
			if (toCompare == "legacy")
			{
				minecraftArguments = " ${auth_player_name} ${auth_session}";
			}
			else if (toCompare == "username_session")
			{
				minecraftArguments = "--username ${auth_player_name} --session ${auth_session}";
			}
			else if (toCompare == "username_session_version")
			{
				minecraftArguments = "--username ${auth_player_name} "
									 "--session ${auth_session} "
									 "--version ${profile_name}";
			}
		}
		if (!minecraftArguments.isEmpty())
		{
			resourceData.overwriteMinecraftArguments = minecraftArguments;
		}
	}
	readString("+minecraftArguments", resourceData.addMinecraftArguments);
	readString("-minecraftArguments", resourceData.removeMinecraftArguments);
	readString("type", out->type);

	parse_timestamp(readStringRet("releaseTime"), out->m_releaseTimeString, out->m_releaseTime);

	QString assetsId;
	readString("assets", assetsId);
	resourceData.assets = std::make_shared<Minecraft::Assets>(assetsId);

	if (root.contains("minimumLauncherVersion"))
	{
		int minimumLauncherVersion = ensureInteger(root, "minimumLauncherVersion");
		if (minimumLauncherVersion > CURRENT_MINIMUM_LAUNCHER_VERSION)
		{
			throw JsonException(
				QString("patch %1 is in a newer format than MultiMC can handle").arg(filename));
		}
	}

	if (root.contains("tweakers"))
	{
		resourceData.shouldOverwriteTweakers = true;
		for (auto tweakerVal : ensureArray(root, "tweakers"))
		{
			resourceData.overwriteTweakers.append(ensureString(tweakerVal));
		}
	}

	if (root.contains("+tweakers"))
	{
		for (auto tweakerVal : ensureArray(root, "+tweakers"))
		{
			resourceData.addTweakers.append(ensureString(tweakerVal));
		}
	}

	if (root.contains("-tweakers"))
	{
		for (auto tweakerVal : ensureArray(root, "-tweakers"))
		{
			resourceData.removeTweakers.append(ensureString(tweakerVal));
		}
	}

	if (root.contains("+traits"))
	{
		for (auto tweakerVal : ensureArray(root, "+traits"))
		{
			resourceData.traits.insert(ensureString(tweakerVal));
		}
	}

	if (root.contains("libraries"))
	{
		resourceData.libraries->shouldOverwriteLibs = true;
		for (auto libVal : ensureArray(root, "libraries"))
		{
			auto libObj = ensureObject(libVal);

			auto lib = readRawLibrary(libObj, filename);
			if (lib->isNative())
			{
				resourceData.natives->overwriteLibs.append(lib);
			}
			else
			{
				resourceData.libraries->overwriteLibs.append(lib);
			}
		}
	}

	QString minecraftVersion;
	readString("id", minecraftVersion);
	if (!minecraftVersion.isEmpty())
	{
		auto libptr = std::make_shared<Library>();
		auto name = QString("net.minecraft:minecraft:%1").arg(minecraftVersion);
		auto url = QString("http://s3.amazonaws.com/Minecraft.Download/versions/%1/%2.jar")
					   .arg(minecraftVersion)
					   .arg(minecraftVersion);
		libptr->setName(GradleSpecifier(name));
		libptr->setAbsoluteUrl(url);
	}

	if (root.contains("+jarMods"))
	{
		for (auto libVal : ensureArray(root, "+jarMods"))
		{
			QJsonObject libObj = ensureObject(libVal);
			// parse the jarmod
			JarmodPtr lib = OneSixFormat::fromJson(libObj, filename);
			// and add to jar mods
			resourceData.jarMods.append(lib);
		}
	}

	if (root.contains("+libraries"))
	{
		for (auto libVal : ensureArray(root, "+libraries"))
		{
			QJsonObject libObj = ensureObject(libVal);
			// parse the library
			auto lib = readRawLibraryPlus(libObj, filename);
			if (lib->isNative())
			{
				resourceData.natives->addLibs.append(lib);
			}
			else
			{
				resourceData.libraries->addLibs.append(lib);
			}
		}
	}

	if (root.contains("-libraries"))
	{
		for (auto libVal : ensureArray(root, "-libraries"))
		{
			auto libObj = ensureObject(libVal);
			resourceData.libraries->removeLibs.append(ensureString(libObj, "name"));
			resourceData.natives->removeLibs.append(ensureString(libObj, "name"));
		}
	}
	return out;
}

JarmodPtr OneSixFormat::fromJson(const QJsonObject &libObj, const QString &filename)
{
	JarmodPtr out(new Jarmod());
	if (!libObj.contains("name"))
	{
		throw JsonException(filename + "contains a jarmod that doesn't have a 'name' field");
	}
	out->name = libObj.value("name").toString();
	return out;
}
