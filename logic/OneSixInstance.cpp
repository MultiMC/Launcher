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
#include "logger/QsLog.h"
#include "MultiMC.h"
#include "MMCError.h"

#include "logic/OneSixInstance.h"

#include "logic/OneSixUpdate.h"
#include "logic/minecraft/InstanceVersion.h"
#include "minecraft/VersionBuildError.h"

#include "logic/assets/AssetsUtils.h"
#include "icons/IconList.h"
#include "logic/MinecraftProcess.h"
#include "gui/pagedialog/PageDialog.h"
#include "gui/pages/VersionPage.h"
#include "gui/pages/ModFolderPage.h"
#include "gui/pages/ResourcePackPage.h"
#include "gui/pages/TexturePackPage.h"
#include "gui/pages/InstanceSettingsPage.h"
#include "gui/pages/NotesPage.h"
#include "gui/pages/ScreenshotsPage.h"
#include "gui/pages/OtherLogsPage.h"

OneSixInstance::OneSixInstance(const QString &rootDir, SettingsObject *settings, QObject *parent)
	: BaseInstance(rootDir, settings, parent)
{
	m_settings->registerSetting("IntendedVersion", "");
	version.reset(new InstanceVersion(this, this));
}

void OneSixInstance::init()
{
	try
	{
		reloadVersion();
	}
	catch (MMCError &e)
	{
		QLOG_ERROR() << "Caught exception on instance init: " << e.cause();
	}
}

QList<BasePage *> OneSixInstance::getPages()
{
	QList<BasePage *> values;
	values.append(new VersionPage(this));
	values.append(new ModFolderPage(this, loaderModList(), "mods", "loadermods",
									tr("Loader mods"), "Loader-mods"));
	values.append(new CoreModFolderPage(this, coreModList(), "coremods", "coremods",
										tr("Core mods"), "Core-mods"));
	values.append(new ResourcePackPage(this));
	values.append(new TexturePackPage(this));
	values.append(new NotesPage(this));
	values.append(new ScreenshotsPage(this));
	values.append(new InstanceSettingsPage(this));
	values.append(new OtherLogsPage(this));
	return values;
}

QString OneSixInstance::dialogTitle()
{
	return tr("Edit Instance (%1)").arg(name());
}

QSet<QString> OneSixInstance::traits()
{
	auto version = getFullVersion();
	if (!version)
	{
		return {"version-incomplete"};
	}
	else
		return version->traits;
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

QDir OneSixInstance::reconstructAssets(std::shared_ptr<InstanceVersion> version)
{
	QDir assetsDir = QDir("assets/");
	QDir indexDir = QDir(PathCombine(assetsDir.path(), "indexes"));
	QDir objectDir = QDir(PathCombine(assetsDir.path(), "objects"));
	QDir virtualDir = QDir(PathCombine(assetsDir.path(), "virtual"));

	QString indexPath = PathCombine(indexDir.path(), version->assets + ".json");
	QFile indexFile(indexPath);
	QDir virtualRoot(PathCombine(virtualDir.path(), version->assets));

	if (!indexFile.exists())
	{
		QLOG_ERROR() << "No assets index file" << indexPath << "; can't reconstruct assets";
		return virtualRoot;
	}

	QLOG_DEBUG() << "reconstructAssets" << assetsDir.path() << indexDir.path()
				 << objectDir.path() << virtualDir.path() << virtualRoot.path();

	AssetsIndex index;
	bool loadAssetsIndex = AssetsUtils::loadAssetsIndexJson(indexPath, &index);

	if (loadAssetsIndex && index.isVirtual)
	{
		QLOG_INFO() << "Reconstructing virtual assets folder at" << virtualRoot.path();

		for (QString map : index.objects.keys())
		{
			AssetObject asset_object = index.objects.value(map);
			QString target_path = PathCombine(virtualRoot.path(), map);
			QFile target(target_path);

			QString tlk = asset_object.hash.left(2);

			QString original_path =
				PathCombine(PathCombine(objectDir.path(), tlk), asset_object.hash);
			QFile original(original_path);
			if (!original.exists())
				continue;
			if (!target.exists())
			{
				QFileInfo info(target_path);
				QDir target_dir = info.dir();
				// QLOG_DEBUG() << target_dir;
				if (!target_dir.exists())
					QDir("").mkpath(target_dir.path());

				bool couldCopy = original.copy(target_path);
				QLOG_DEBUG() << " Copying" << original_path << "to" << target_path
							 << QString::number(couldCopy); // << original.errorString();
			}
		}

		// TODO: Write last used time to virtualRoot/.lastused
	}

	return virtualRoot;
}

QStringList OneSixInstance::processMinecraftArgs(AuthSessionPtr session)
{
	QString args_pattern = version->minecraftArguments;
	for (auto tweaker : version->tweakers)
	{
		args_pattern += " --tweakClass " + tweaker;
	}

	QMap<QString, QString> token_mapping;
	// yggdrasil!
	token_mapping["auth_username"] = session->username;
	token_mapping["auth_session"] = session->session;
	token_mapping["auth_access_token"] = session->access_token;
	token_mapping["auth_player_name"] = session->player_name;
	token_mapping["auth_uuid"] = session->uuid;

	// these do nothing and are stupid.
	token_mapping["profile_name"] = name();
	token_mapping["version_name"] = version->id;

	QString absRootDir = QDir(minecraftRoot()).absolutePath();
	token_mapping["game_directory"] = absRootDir;
	QString absAssetsDir = QDir("assets/").absolutePath();
	token_mapping["game_assets"] = reconstructAssets(version).absolutePath();

	token_mapping["user_properties"] = session->serializeUserProperties();
	token_mapping["user_type"] = session->user_type;
	// 1.7.3+ assets tokens
	token_mapping["assets_root"] = absAssetsDir;
	token_mapping["assets_index_name"] = version->assets;

	QStringList parts = args_pattern.split(' ', QString::SkipEmptyParts);
	for (int i = 0; i < parts.length(); i++)
	{
		parts[i] = replaceTokensIn(parts[i], token_mapping);
	}
	return parts;
}

bool OneSixInstance::prepareForLaunch(AuthSessionPtr session, QString &launchScript)
{

	QIcon icon = MMC->icons()->getIcon(iconKey());
	auto pixmap = icon.pixmap(128, 128);
	pixmap.save(PathCombine(minecraftRoot(), "icon.png"), "PNG");

	if (!version)
		return nullptr;

	// libraries and class path.
	{
		auto libs = version->getActiveNormalLibs();
		for (auto lib : libs)
		{
			launchScript += "cp " + librariesPath().absoluteFilePath(lib->storagePath()) + "\n";
		}
		if (version->hasJarMods())
		{
			launchScript += "cp " + QDir(instanceRoot()).absoluteFilePath("temp.jar") + "\n";
		}
		else
		{
			QString relpath = version->id + "/" + version->id + ".jar";
			launchScript += "cp " + versionsPath().absoluteFilePath(relpath) + "\n";
		}
	}
	if (!version->mainClass.isEmpty())
	{
		launchScript += "mainClass " + version->mainClass + "\n";
	}
	if (!version->appletClass.isEmpty())
	{
		launchScript += "appletClass " + version->appletClass + "\n";
	}

	// generic minecraft params
	for (auto param : processMinecraftArgs(session))
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
		launchScript += "userName " + session->player_name + "\n";
		launchScript += "sessionId " + session->session + "\n";
	}

	// native libraries (mostly LWJGL)
	{
		QDir natives_dir(PathCombine(instanceRoot(), "natives/"));
		for (auto native : version->getActiveNativeLibs())
		{
			QFileInfo finfo(PathCombine("libraries", native->storagePath()));
			launchScript += "ext " + finfo.absoluteFilePath() + "\n";
		}
		launchScript += "natives " + natives_dir.absolutePath() + "\n";
	}

	// traits. including legacyLaunch and others ;)
	for (auto trait : version->traits)
	{
		launchScript += "traits " + trait + "\n";
	}
	launchScript += "launcher onesix\n";
	return true;
}

void OneSixInstance::cleanupAfterRun()
{
	QString target_dir = PathCombine(instanceRoot(), "natives/");
	QDir dir(target_dir);
	dir.removeRecursively();
}

std::shared_ptr<ModList> OneSixInstance::loaderModList()
{
	if (!loader_mod_list)
	{
		loader_mod_list.reset(new ModList(loaderModsDir()));
	}
	loader_mod_list->update();
	return loader_mod_list;
}

std::shared_ptr<ModList> OneSixInstance::coreModList()
{
	if (!core_mod_list)
	{
		core_mod_list.reset(new ModList(coreModsDir()));
	}
	core_mod_list->update();
	return core_mod_list;
}

std::shared_ptr<ModList> OneSixInstance::resourcePackList()
{
	if (!resource_pack_list)
	{
		resource_pack_list.reset(new ModList(resourcePacksDir()));
	}
	resource_pack_list->update();
	return resource_pack_list;
}

std::shared_ptr<ModList> OneSixInstance::texturePackList()
{
	if (!texture_pack_list)
	{
		texture_pack_list.reset(new ModList(texturePacksDir()));
	}
	texture_pack_list->update();
	return texture_pack_list;
}

bool OneSixInstance::setIntendedVersionId(QString version)
{
	settings().set("IntendedVersion", version);
	QFile::remove(PathCombine(instanceRoot(), "version.json"));
	clearVersion();
	return true;
}

QString OneSixInstance::intendedVersionId() const
{
	return settings().get("IntendedVersion").toString();
}

void OneSixInstance::setShouldUpdate(bool)
{
}

bool OneSixInstance::shouldUpdate() const
{
	return true;
}

bool OneSixInstance::versionIsCustom()
{
	if (version)
	{
		return !version->isVanilla();
	}
	return false;
}

bool OneSixInstance::versionIsFTBPack()
{
	if (version)
	{
		return version->hasFtbPack();
	}
	return false;
}

QString OneSixInstance::currentVersionId() const
{
	return intendedVersionId();
}

void OneSixInstance::reloadVersion()
{

	try
	{
		version->reload(externalPatches());
		unsetFlag(VersionBrokenFlag);
		emit versionReloaded();
	}
	catch (VersionIncomplete &error)
	{
	}
	catch (MMCError &error)
	{
		version->clear();
		setFlag(VersionBrokenFlag);
		// TODO: rethrow to show some error message(s)?
		emit versionReloaded();
		throw;
	}
}

void OneSixInstance::clearVersion()
{
	version->clear();
	emit versionReloaded();
}

std::shared_ptr<InstanceVersion> OneSixInstance::getFullVersion() const
{
	return version;
}

QString OneSixInstance::getStatusbarDescription()
{
	QStringList traits;
	if (versionIsCustom())
	{
		traits.append(tr("custom"));
	}
	if (flags() & VersionBrokenFlag)
	{
		traits.append(tr("broken"));
	}

	if (traits.size())
	{
		return tr("Minecraft %1 (%2)").arg(intendedVersionId()).arg(traits.join(", "));
	}
	else
	{
		return tr("Minecraft %1").arg(intendedVersionId());
	}
}

QDir OneSixInstance::librariesPath() const
{
	return QDir::current().absoluteFilePath("libraries");
}

QDir OneSixInstance::jarmodsPath() const
{
	return QDir(jarModsDir());
}

QDir OneSixInstance::versionsPath() const
{
	return QDir::current().absoluteFilePath("versions");
}

QStringList OneSixInstance::externalPatches() const
{
	return QStringList();
}

bool OneSixInstance::providesVersionFile() const
{
	return false;
}

bool OneSixInstance::reload()
{
	if (BaseInstance::reload())
	{
		try
		{
			reloadVersion();
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
	auto version = getFullVersion();
	if (!version)
		return list;
	if (version->hasJarMods())
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
