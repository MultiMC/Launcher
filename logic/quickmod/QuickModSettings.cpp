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

#include "QuickModSettings.h"

#include <QMap>
#include <QDir>

#include "logic/settings/INISettingsObject.h"
#include "logic/settings/Setting.h"
#include "QuickModMetadata.h"
#include "QuickModVersionRef.h"
#include "QuickModRef.h"
#include "logic/BaseInstance.h"

QuickModSettings::QuickModSettings()
	: m_settings(new INISettingsObject(QDir::current().absoluteFilePath("quickmod.cfg")))
{
	m_settings->registerSetting("AvailableMods",
								QVariant::fromValue(QMap<QString, QMap<QString, QString>>()));
	m_settings->registerSetting("TrustedWebsites", QVariantList());
	m_settings->registerSetting("Indices", QVariantMap());
}

QuickModSettings::~QuickModSettings()
{
	delete m_settings;
}

void QuickModSettings::markModAsExists(QuickModMetadataPtr mod, const QuickModVersionRef &version,
									   const QString &fileName)
{
	auto mods = m_settings->get("AvailableMods").toMap();
	auto map = mods[mod->internalUid()].toMap();
	map[version.toString()] = fileName;
	mods[mod->internalUid()] = map;
	m_settings->getSetting("AvailableMods")->set(QVariant(mods));
}

void QuickModSettings::markModAsInstalled(const QuickModRef uid,
										  const QuickModVersionRef &version,
										  const QString &fileName, InstancePtr instance)
{
	auto mods = instance->settings().get("InstalledMods").toMap();
	auto map = mods[uid.toString()].toMap();
	map[version.toString()] = fileName;
	mods[uid.toString()] = map;
	instance->settings().getSetting("InstalledMods")->set(QVariant(mods));
}
void QuickModSettings::markModAsUninstalled(const QuickModRef uid,
											const QuickModVersionRef &version,
											InstancePtr instance)
{
	auto mods = instance->settings().get("InstalledMods").toMap();
	if (version.findVersion())
	{
		auto map = mods[uid.toString()].toMap();
		map.remove(version.toString());
		if (map.isEmpty())
		{
			mods.remove(uid.toString());
		}
		else
		{
			mods[uid.toString()] = map;
		}
	}
	else
	{
		mods.remove(uid.toString());
	}
	instance->settings().set("InstalledMods", QVariant(mods));
}
bool QuickModSettings::isModMarkedAsInstalled(const QuickModRef uid,
											  const QuickModVersionRef &version,
											  InstancePtr instance) const
{
	auto mods = instance->settings().get("InstalledMods").toMap();
	if (!version.findVersion())
	{
		return mods.contains(uid.toString());
	}
	return mods.contains(uid.toString()) &&
		   mods.value(uid.toString()).toMap().contains(version.toString());
}
bool QuickModSettings::isModMarkedAsExists(QuickModMetadataPtr mod,
										   const QuickModVersionRef &version) const
{
	if (!version.findVersion())
	{
		return m_settings->get("AvailableMods").toMap().contains(mod->internalUid());
	}
	auto mods = m_settings->get("AvailableMods").toMap();
	return mods.contains(mod->internalUid()) &&
		   mods.value(mod->internalUid()).toMap().contains(version.toString());
}
QMap<QuickModVersionRef, QString>
QuickModSettings::installedModFiles(const QuickModRef uid, BaseInstance *instance) const
{
	auto mods = instance->settings().get("InstalledMods").toMap();
	auto tmp = mods[uid.toString()].toMap();
	QMap<QuickModVersionRef, QString> out;
	for (auto it = tmp.begin(); it != tmp.end(); ++it)
	{
		out.insert(QuickModVersionRef(uid, it.key()), it.value().toString());
	}
	return out;
}
QString QuickModSettings::existingModFile(QuickModMetadataPtr mod,
										  const QuickModVersionRef &version) const
{
	if (!isModMarkedAsExists(mod, version))
	{
		return QString();
	}
	auto mods = m_settings->get("AvailableMods").toMap();
	return mods[mod->internalUid()].toMap()[version.toString()].toString();
}
