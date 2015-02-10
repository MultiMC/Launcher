#include "WonkoPackageVersion.h"

#include "wonko/DownloadableResource.h"
#include "minecraft/Libraries.h"
#include "minecraft/Assets.h"
#include "Json.h"
#include "Rules.h"

void WonkoPackageVersion::load(const QJsonObject &obj, const QString &uid)
{
	using namespace Json;

	const int formatVersion = ensureInteger(obj, "formatVersion", 0);

	//////////////// METADATA ////////////////////////

	m_uid = uid.isEmpty() ? ensureString(obj, "uid") : ensureString(obj, "uid", uid);
	m_id = ensureString(obj, "version");
	m_time = QDateTime::fromMSecsSinceEpoch(ensureDouble(obj, "time") * 1000);
	m_type = ensureString(obj, "type", "");

	if (obj.contains("requires"))
	{
		for (const QJsonObject &item : ensureIsArrayOf<QJsonObject>(obj, "requires"))
		{
			const QString uid = ensureString(item, "uid");
			const QString version = ensureString(obj, "version", QString());
			m_dependencies[uid] = version;
		}
	}

	//////////////// ACTUAL DATA ///////////////////////

#define FACTORY(CLAZZ, ...) std::make_shared<CLAZZ>(__VA_ARGS__)

	QList<std::shared_ptr<BaseResourceFactory>> factories;
	factories << FACTORY(StringListResourceFactory, 0, "general.traits")
			  << FACTORY(FoldersResourceFactory)
			  << FACTORY(Minecraft::LibrariesFactory, 0, "java.libraries")
			  << FACTORY(Minecraft::LibrariesFactory, 0, "java.natives")
			  << FACTORY(StringResourceFactory, 0, "java.mainClass")
			  << FACTORY(StringResourceFactory, 0, "mc.appletClass")
			  << FACTORY(Minecraft::AssetsFactory)
			  << FACTORY(StringResourceFactory, 0, "mc.arguments")
			  << FACTORY(StringListResourceFactory, 0, "mc.tweakers");

	// sort the factories for easier access
	QMap<QString, QList<std::shared_ptr<BaseResourceFactory>>> resourceFactories;
	for (auto factory : factories)
	{
		for (const QString &key : factory->keys(formatVersion))
		{
			resourceFactories[key].append(factory);
		}
	}

	if (obj.contains("data"))
	{
		QJsonObject commonObj;
		QJsonObject clientObj;
		for (const QJsonObject &group : ensureIsArrayOf<QJsonObject>(obj, "data"))
		{
			Rules rules;
			rules.load(group.contains("rules") ? group.value("rules") : QJsonArray());
			if (rules.result(RuleContext{"client"}) == BaseRule::Allow)
			{
				clientObj = group;
			}
			else if (rules.result(RuleContext{""}) == BaseRule::Allow)
			{
				commonObj = group;
			}
		}

		QMap<QString, ResourcePtr> result;
		auto loadGroup = [&result, resourceFactories, formatVersion](const QJsonObject &resources)
		{
			for (const QString &key : resources.keys())
			{
				bool factoryFound = false;
				if (resourceFactories.contains(key))
				{
					for (auto factory : resourceFactories[key])
					{
						if (factory->supportsFormatVersion(formatVersion))
						{
							ResourcePtr ptr = factory->create(formatVersion, key, ensureJsonValue(resources, key));
							if (result.contains(key))
							{
								ptr->applyTo(result[key]);
							}
							else
							{
								result.insert(key, ptr);
							}
							factoryFound = true;
							break;
						}
					}
				}
				if (!factoryFound)
				{
					qWarning() << "No factory found for" << key << "version" << formatVersion;
				}
			}
		};
		loadGroup(commonObj);
		loadGroup(clientObj);
		m_resources = result;
	}
}
