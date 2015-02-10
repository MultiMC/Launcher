#include "OneSixFormat.h"

#include "minecraft/Package.h"
#include "wonko/Rules.h"
#include "Json.h"

using namespace Json;

QJsonObject OneSixFormat::toJson(std::shared_ptr<ImplicitRule> rule)
{
	QJsonObject ruleObj;
	ruleObj.insert("action", rule->resultToString());
	return ruleObj;
}

QJsonObject OneSixFormat::toJson(std::shared_ptr<OsRule> rule)
{
	QJsonObject ruleObj;
	ruleObj.insert("action", rule->resultToString());
	QJsonObject osObj;
	{
		osObj.insert("name", rule->m_system.toString());
		osObj.insert("version", rule->m_version_regexp);
	}
	ruleObj.insert("os", osObj);
	return ruleObj;
}

QJsonObject OneSixFormat::toJson(LibraryPtr raw)
{
	QJsonObject libRoot;
	libRoot.insert("name", (QString)raw->m_name);
	if (!raw->m_absolute_url.isEmpty())
		libRoot.insert("MMC-absoluteUrl", Json::toJson(raw->m_absolute_url));
	if (!raw->m_hint.isEmpty())
		libRoot.insert("MMC-hint", raw->m_hint);
	if (raw->m_base_url != "http://" + URLConstants::AWS_DOWNLOAD_LIBRARIES &&
		raw->m_base_url != "https://" + URLConstants::AWS_DOWNLOAD_LIBRARIES &&
		raw->m_base_url != "https://" + URLConstants::LIBRARY_BASE && !raw->m_base_url.isEmpty())
	{
		libRoot.insert("url", Json::toJson(raw->m_base_url));
	}
	if (raw->isNative())
	{
		QJsonObject nativeList;
		auto iter = raw->m_native_classifiers.begin();
		while (iter != raw->m_native_classifiers.end())
		{
			nativeList.insert(iter.key().toString(), iter.value());
			iter++;
		}
		libRoot.insert("natives", nativeList);
		if (raw->extract_excludes.size())
		{
			QJsonArray excludes;
			QJsonObject extract;
			for (auto exclude : raw->extract_excludes)
			{
				excludes.append(exclude);
			}
			extract.insert("exclude", excludes);
			libRoot.insert("extract", extract);
		}
	}
	if (raw->m_rules)
	{
		QJsonArray allRules;
		for (auto &rule : raw->m_rules->rules())
		{
			QJsonObject ruleObj;
			auto implicitRule = std::dynamic_pointer_cast<ImplicitRule>(rule);
			auto osRule = std::dynamic_pointer_cast<ImplicitRule>(rule);
			if(implicitRule)
			{
				ruleObj = toJson(implicitRule);
				allRules.append(ruleObj);
			}
			else if(osRule)
			{
				ruleObj = toJson(implicitRule);
				allRules.append(ruleObj);
			}
			else
			{
				qWarning() << "Couldn't recognize rule type!";
			}
		}
		libRoot.insert("rules", allRules);
	}
	return libRoot;
}

static QList<LibraryPtr> toOneSixLibraries(const QList<LibraryPtr> &libraries)
{
	QList<LibraryPtr> out;
	for (const LibraryPtr &ptr : libraries)
	{
		out.append(std::dynamic_pointer_cast<Library>(ptr));
	}
	return out;
}
QJsonDocument OneSixFormat::toJson(PackagePtr file, bool saveOrder)
{
	QJsonObject root;
	if (saveOrder)
	{
		root.insert("order", file->getOrder());
	}
	writeString(root, "name", file->name);
	writeString(root, "fileId", file->fileId);
	writeString(root, "version", file->version);
	if(file->dependencies.contains("net.minecraft"))
	{
		writeString(root, "mcVersion", file->dependencies["net.minecraft"]);
	}

	auto & resourceData = file->resources;
	writeString(root, "mainClass", resourceData.mainClass);
	writeString(root, "appletClass", resourceData.appletClass);
	writeString(root, "minecraftArguments", resourceData.overwriteMinecraftArguments);
	writeString(root, "+minecraftArguments", resourceData.addMinecraftArguments);
	writeString(root, "-minecraftArguments", resourceData.removeMinecraftArguments);
	writeString(root, "type", file->type);
	if(resourceData.assets)
	{
		writeString(root, "assets", resourceData.assets->id());
	}
	if (file->fileId == "net.minecraft")
	{
		writeString(root, "releaseTime", file->m_releaseTimeString);
		root.insert("minimumLauncherVersion", CURRENT_MINIMUM_LAUNCHER_VERSION);
	}
	writeStringList(root, "tweakers", resourceData.overwriteTweakers);
	writeStringList(root, "+tweakers", resourceData.addTweakers);
	writeStringList(root, "-tweakers", resourceData.removeTweakers);
	writeStringList(root, "+traits", resourceData.traits.toList());
	writeObjectList<OneSixFormat>(root, "libraries", toOneSixLibraries(resourceData.libraries->overwriteLibs +
																	   resourceData.natives->overwriteLibs));
	if (!resourceData.libraries->addLibs.isEmpty() || !resourceData.natives->addLibs.isEmpty())
	{
		QJsonArray array;
		for(auto plusLib: (resourceData.libraries->addLibs + resourceData.natives->addLibs))
		{
			// filter out the 'minecraft version'
			if(plusLib->name().artifactPrefix() == "net.minecraft:minecraft")
			{
				// and write it in the old format instead
				writeString(root, "id", plusLib->name().version());
				continue;
			}
			array.append(OneSixFormat::toJson(std::dynamic_pointer_cast<Library>(plusLib)));
		}
		// we could have removed minecraft from the array of libs. Do not write empty array.
		if(!array.isEmpty())
		{
			root.insert("+libraries", array);
		}
	}
	if (!resourceData.libraries->removeLibs.isEmpty() || !resourceData.natives->removeLibs.isEmpty())
	{
		QJsonArray array;
		for (auto lib : (resourceData.libraries->removeLibs + resourceData.natives->removeLibs))
		{
			QJsonObject rmlibobj;
			rmlibobj.insert("name", lib);
			array.append(rmlibobj);
		}
		root.insert("-libraries", array);
	}
	writeObjectList<OneSixFormat>(root, "+jarMods", resourceData.jarMods);
	// write the contents to a json document.
	{
		QJsonDocument out;
		out.setObject(root);
		return out;
	}
}

QJsonObject OneSixFormat::toJson(JarmodPtr mod)
{
	QJsonObject out;
	writeString(out, "name", mod->name);
	return out;
}
