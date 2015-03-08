/* Copyright 2013-2015 MultiMC Contributors
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

#include <QIcon>
#include <pathutils.h>
#include <QDebug>
#include "Exception.h"

#include "minecraft/onesix/OneSixInstance.h"
#include "minecraft/onesix/OneSixUpdate.h"
#include "minecraft/onesix/OneSixProfileStrategy.h"

#include "auth/minecraft/MojangAuthSession.h"
#include "minecraft/MinecraftProfile.h"
#include "minecraft/VersionBuildError.h"
#include "minecraft/Process.h"
#include "minecraft/Assets.h"
#include "MMCZip.h"

#include "icons/IconList.h"

OneSixInstance::OneSixInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir)
	: MinecraftInstance(globalSettings, settings, rootDir)
{
	m_settings->registerSetting({"MinecraftVersion", "IntendedVersion"}, "");
	m_settings->registerSetting("LWJGLVersion", "2.9.1");
	m_settings->registerSetting("ForgeVersion", "");
	m_settings->registerSetting("LiteLoaderVersion", "");
}

QString OneSixInstance::minecraftVersion() const
{
	return m_settings->get("MinecraftVersion").toString();
}

QString OneSixInstance::lwjglVersion() const
{
	return m_settings->get("LWJGLVersion").toString();
}

QString OneSixInstance::forgeVersion() const
{
	return m_settings->get("ForgeVersion").toString();
}

QString OneSixInstance::liteloaderVersion() const
{
	return m_settings->get("LiteLoaderVersion").toString();
}

void OneSixInstance::setMinecraftVersion(QString version)
{
	m_settings->set("MinecraftVersion", version);
	emit propertiesChanged(this);
}

void OneSixInstance::setLwjglVersion(QString version)
{
	m_settings->set("LWJGLVersion", version);
}

void OneSixInstance::setForgeVersion(QString version)
{
	m_settings->set("ForgeVersion", version);
}

void OneSixInstance::setLiteloaderVersion(QString version)
{
	m_settings->set("LiteLoaderVersion", version);
}

void OneSixInstance::init()
{
	createProfile();
}

void OneSixInstance::createProfile()
{
	m_version.reset(new MinecraftProfile(new OneSixProfileStrategy(this)));
}

QSet<QString> OneSixInstance::traits()
{
	auto version = getMinecraftProfile();
	if (!version)
	{
		return {"version-incomplete"};
	}
	else
		return version->resources.traits;
}

std::shared_ptr<Task> OneSixInstance::doUpdate()
{
	return std::shared_ptr<Task>(new OneSixUpdate(this));
}

QString replaceTokensIn(QString text, QMap<QString, QString> with)
{
	QString result;
	QRegExp token_regexp("\\$\\{(.+)\\}");
	token_regexp.setMinimal(true);
	QStringList list;
	int tail = 0;
	int head = 0;
	while ((head = token_regexp.indexIn(text, head)) != -1)
	{
		result.append(text.mid(tail, head - tail));
		QString key = token_regexp.cap(1);
		auto iter = with.find(key);
		if (iter != with.end())
		{
			result.append(*iter);
		}
		head += token_regexp.matchedLength();
		tail = head;
	}
	result.append(text.mid(tail));
	return result;
}

QStringList OneSixInstance::processMinecraftArgs(MojangAuthSessionPtr acc)
{
	QString args_pattern = m_version->resources.minecraftArguments;
	for (auto tweaker : m_version->resources.tweakers)
	{
		args_pattern += " --tweakClass " + tweaker;
	}

	QMap<QString, QString> token_mapping;
	// yggdrasil!
	token_mapping["auth_username"] = acc->username;
	token_mapping["auth_session"] = acc->session;
	token_mapping["auth_access_token"] = acc->access_token;
	token_mapping["auth_player_name"] = acc->player_name;
	token_mapping["auth_uuid"] = acc->uuid;

	// blatant self-promotion.
	token_mapping["profile_name"] = token_mapping["version_name"] = "MultiMC5";

	QString absRootDir = QDir(minecraftRoot()).absolutePath();
	token_mapping["game_directory"] = absRootDir;
	QString absAssetsDir = QDir("assets/").absolutePath();
	token_mapping["game_assets"] = m_version->resources.assets->storageFolder();

	token_mapping["user_properties"] = acc->serializeUserProperties();
	token_mapping["user_type"] = acc->user_type;
	// 1.7.3+ assets tokens
	token_mapping["assets_root"] = absAssetsDir;
	token_mapping["assets_index_name"] = m_version->resources.assets->id();

	QStringList parts = args_pattern.split(' ', QString::SkipEmptyParts);
	for (int i = 0; i < parts.length(); i++)
	{
		parts[i] = replaceTokensIn(parts[i], token_mapping);
	}
	return parts;
}

// FIXME: this should be split and turned into a task
BaseProcess *OneSixInstance::prepareForLaunch(SessionPtr acc)
{
	MojangAuthSessionPtr account = std::dynamic_pointer_cast<MojangAuthSession>(acc);

	QString launchScript;
	QIcon icon = ENV.icons()->getIcon(iconKey());
	auto pixmap = icon.pixmap(128, 128);
	pixmap.save(PathCombine(minecraftRoot(), "icon.png"), "PNG");

	if (!m_version)
		return nullptr;

	auto libs = m_version->resources.libraries->getActiveLibs();
	for (auto lib : libs)
	{
		// FIXME: stupid hardcoded thing
		if(lib->name().artifactPrefix() == "net.minecraft:minecraft")
		{
			if(!m_version->resources.jarMods.isEmpty())
			{
				launchScript += "cp " + QDir(instanceRoot()).absoluteFilePath("temp.jar") + "\n";
				continue;
			}
		}
		launchScript += "cp " + QDir::current().absoluteFilePath(lib->storagePath()) + "\n";
	}

	// create temporary modded jar, if needed
	QList<Mod> jarMods;
	for (auto jarmod : m_version->resources.jarMods)
	{
		QString filePath = jarmodsPath().absoluteFilePath(jarmod->name);
		jarMods.push_back(Mod(QFileInfo(filePath)));
	}
	if(jarMods.size())
	{
		auto finalJarPath = QDir(instanceRoot()).absoluteFilePath("temp.jar");
		QFile finalJar(finalJarPath);
		if(finalJar.exists())
		{
			if(!finalJar.remove())
			{
				//FIXME: return errors from here.
				qCritical() << QString("Couldn't remove stale jar file: %1").arg(finalJarPath);
				// emitFailed(tr("Couldn't remove stale jar file: %1").arg(finalJarPath));
				return nullptr;
			}
		}
		auto libs = m_version->resources.libraries->getActiveLibs();
		QString sourceJarPath;

		// find net.minecraft:minecraft
		for(auto foo:libs)
		{
			// FIXME: stupid hardcoded thing
			if(foo->name().artifactPrefix() == "net.minecraft:minecraft")
			{
				sourceJarPath = foo->storagePath();
				break;
			}
		}

		// use it as the source for jar modding
		if(!sourceJarPath.isNull() && !MMCZip::createModdedJar(sourceJarPath, finalJarPath, jarMods))
		{
			//FIXME: return errors from here.
			qCritical() << QString("Failed to create the custom Minecraft jar file.");
			// emitFailed(tr("Failed to create the custom Minecraft jar file."));
			return nullptr;
		}
	}

	if (!m_version->resources.mainClass.isEmpty())
	{
		launchScript += "mainClass " + m_version->resources.mainClass + "\n";
	}

	if (!m_version->resources.appletClass.isEmpty())
	{
		launchScript += "appletClass " + m_version->resources.appletClass + "\n";
	}

	// generic minecraft params
	for (auto param : processMinecraftArgs(account))
	{
		launchScript += "param " + param + "\n";
	}

	// window size, title and state, legacy
	{
		QString windowParams;
		if (settings().get("LaunchMaximized").toBool())
			windowParams = "max";
		else
			windowParams = QString("%1x%2")
							   .arg(settings().get("MinecraftWinWidth").toInt())
							   .arg(settings().get("MinecraftWinHeight").toInt());
		launchScript += "windowTitle " + windowTitle() + "\n";
		launchScript += "windowParams " + windowParams + "\n";
	}

	// legacy auth
	{
		launchScript += "userName " + account->player_name + "\n";
		launchScript += "sessionId " + account->session + "\n";
	}

	// native libraries (mostly LWJGL)
	{
		QDir natives_dir(PathCombine(instanceRoot(), "natives/"));
		for (auto native : m_version->resources.natives->getActiveLibs())
		{
			QFileInfo finfo(native->storagePath());
			launchScript += "ext " + finfo.absoluteFilePath() + "\n";
		}
		launchScript += "natives " + natives_dir.absolutePath() + "\n";
	}

	// traits. including legacyLaunch and others ;)
	for (auto trait : m_version->resources.traits)
	{
		launchScript += "traits " + trait + "\n";
	}
	launchScript += "launcher onesix\n";

	qDebug() << launchScript;
	auto process = Minecraft::Process::create(std::dynamic_pointer_cast<MinecraftInstance>(getSharedPtr()));
	process->setLaunchScript(launchScript);
	process->setWorkdir(minecraftRoot());
	process->setLogin(account);
	return process;
}

void OneSixInstance::cleanupAfterRun()
{
	QString target_dir = PathCombine(instanceRoot(), "natives/");
	QDir dir(target_dir);
	dir.removeRecursively();
}

std::shared_ptr<ModList> OneSixInstance::loaderModList() const
{
	if (!m_loader_mod_list)
	{
		m_loader_mod_list.reset(new ModList(loaderModsDir()));
	}
	m_loader_mod_list->update();
	return m_loader_mod_list;
}

std::shared_ptr<ModList> OneSixInstance::coreModList() const
{
	if (!m_core_mod_list)
	{
		m_core_mod_list.reset(new ModList(coreModsDir()));
	}
	m_core_mod_list->update();
	return m_core_mod_list;
}

std::shared_ptr<ModList> OneSixInstance::resourcePackList() const
{
	if (!m_resource_pack_list)
	{
		m_resource_pack_list.reset(new ModList(resourcePacksDir()));
	}
	m_resource_pack_list->update();
	return m_resource_pack_list;
}

std::shared_ptr<ModList> OneSixInstance::texturePackList() const
{
	if (!m_texture_pack_list)
	{
		m_texture_pack_list.reset(new ModList(texturePacksDir()));
	}
	m_texture_pack_list->update();
	return m_texture_pack_list;
}

void OneSixInstance::reloadProfile()
{
	try
	{
		m_version->reload();
		unsetFlag(VersionBrokenFlag);
		emit versionReloaded();
	}
	catch (VersionIncomplete &error)
	{
	}
	catch (Exception &error)
	{
		m_version->clear();
		setFlag(VersionBrokenFlag);
		// TODO: rethrow to show some error message(s)?
		emit versionReloaded();
		throw;
	}
}

void OneSixInstance::clearProfile()
{
	m_version->clear();
	emit versionReloaded();
}

std::shared_ptr<MinecraftProfile> OneSixInstance::getMinecraftProfile() const
{
	return m_version;
}

QString OneSixInstance::getStatusbarDescription()
{
	QStringList traits;
	if (flags() & VersionBrokenFlag)
	{
		traits.append(tr("broken"));
	}

	if (traits.size())
	{
		return tr("Minecraft %1 (%2)").arg(minecraftVersion()).arg(traits.join(", "));
	}
	else
	{
		return tr("Minecraft %1").arg(minecraftVersion());
	}
}

QDir OneSixInstance::jarmodsPath() const
{
	return QDir(jarModsDir());
}

bool OneSixInstance::reload()
{
	if (BaseInstance::reload())
	{
		try
		{
			reloadProfile();
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
	return false;
}

QString OneSixInstance::loaderModsDir() const
{
	return PathCombine(minecraftRoot(), "mods");
}

QString OneSixInstance::coreModsDir() const
{
	return PathCombine(minecraftRoot(), "coremods");
}

QString OneSixInstance::resourcePacksDir() const
{
	return PathCombine(minecraftRoot(), "resourcepacks");
}

QString OneSixInstance::texturePacksDir() const
{
	return PathCombine(minecraftRoot(), "texturepacks");
}

QString OneSixInstance::instanceConfigFolder() const
{
	return PathCombine(minecraftRoot(), "config");
}

QString OneSixInstance::jarModsDir() const
{
	return PathCombine(instanceRoot(), "jarmods");
}

QString OneSixInstance::libDir() const
{
	return PathCombine(minecraftRoot(), "lib");
}

QStringList OneSixInstance::extraArguments() const
{
	auto list = BaseInstance::extraArguments();
	auto version = getMinecraftProfile();
	if (!version)
		return list;
	if (!version->resources.jarMods.isEmpty())
	{
		list.append({"-Dfml.ignoreInvalidMinecraftCertificates=true",
					 "-Dfml.ignorePatchDiscrepancies=true"});
	}
	return list;
}

std::shared_ptr<OneSixInstance> OneSixInstance::getSharedPtr()
{
	return std::dynamic_pointer_cast<OneSixInstance>(BaseInstance::getSharedPtr());
}
