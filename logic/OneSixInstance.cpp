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

#include "OneSixInstance.h"

#include <QIcon>

#include "OneSixInstance_p.h"
#include "OneSixUpdate.h"
#include "OneSixVersion.h"
#include "pathutils.h"
#include "logger/QsLog.h"
#include "assets/AssetsUtils.h"
#include "MultiMC.h"
#include "icons/IconList.h"
#include "MinecraftProcess.h"
#include "gui/dialogs/OneSixModEditDialog.h"

OneSixInstance::OneSixInstance(const QString &rootDir, SettingsObject *settings, QObject *parent)
	: BaseInstance(new OneSixInstancePrivate(), rootDir, settings, parent)
{
	I_D(OneSixInstance);
	d->m_settings->registerSetting("IntendedVersion", "");
	d->m_settings->registerSetting("ShouldUpdate", false);
	d->version.reset(new OneSixVersion(this, this));
	d->vanillaVersion.reset(new OneSixVersion(this, this));
}

void OneSixInstance::init()
{
	if (QDir(instanceRoot()).exists("version.json"))
	{
		reloadVersion();
	}
	else
	{
		clearVersion();
	}
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

QDir OneSixInstance::reconstructAssets(std::shared_ptr<OneSixVersion> version)
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
			if(!original.exists())
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
	I_D(OneSixInstance);
	auto version = d->version;
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
	token_mapping["game_assets"] = reconstructAssets(d->version).absolutePath();

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

MinecraftProcess *OneSixInstance::prepareForLaunch(AuthSessionPtr session)
{
	I_D(OneSixInstance);

	QIcon icon = MMC->icons()->getIcon(iconKey());
	auto pixmap = icon.pixmap(128, 128);
	pixmap.save(PathCombine(minecraftRoot(), "icon.png"), "PNG");

	auto version = d->version;
	if (!version)
		return nullptr;
	QString launchScript;
	{
		auto libs = version->getActiveNormalLibs();
		for (auto lib : libs)
		{
			launchScript += "cp " + librariesPath().absoluteFilePath(lib->storagePath()) + "\n";
		}
		QString targetstr = version->id + "/" + version->id + ".jar";
		launchScript += "cp " + versionsPath().absoluteFilePath(targetstr) + "\n";
	}
	launchScript += "mainClass " + version->mainClass + "\n";

	for (auto param : processMinecraftArgs(session))
	{
		launchScript += "param " + param + "\n";
	}

	// Set the width and height for 1.6 instances
	bool maximize = settings().get("LaunchMaximized").toBool();
	if (maximize)
	{
		// this is probably a BAD idea
		// launchScript += "param --fullscreen\n";
	}
	else
	{
		launchScript +=
			"param --width\nparam " + settings().get("MinecraftWinWidth").toString() + "\n";
		launchScript +=
			"param --height\nparam " + settings().get("MinecraftWinHeight").toString() + "\n";
	}
	QDir natives_dir(PathCombine(instanceRoot(), "natives/"));
	launchScript += "windowTitle " + windowTitle() + "\n";
	for(auto native: version->getActiveNativeLibs())
	{
		QFileInfo finfo(PathCombine("libraries", native->storagePath()));
		launchScript += "ext " + finfo.absoluteFilePath() + "\n";
	}
	launchScript += "natives " + natives_dir.absolutePath() + "\n";
	launchScript += "launch onesix\n";

	// create the process and set its parameters
	MinecraftProcess *proc = new MinecraftProcess(this);
	proc->setWorkdir(minecraftRoot());
	proc->setLaunchScript(launchScript);
	// proc->setNativeFolder(natives_dir.absolutePath());
	return proc;
}

void OneSixInstance::cleanupAfterRun()
{
	QString target_dir = PathCombine(instanceRoot(), "natives/");
	QDir dir(target_dir);
	dir.removeRecursively();
}

std::shared_ptr<ModList> OneSixInstance::loaderModList()
{
	I_D(OneSixInstance);
	if (!d->loader_mod_list)
	{
		d->loader_mod_list.reset(new ModList(loaderModsDir()));
	}
	d->loader_mod_list->update();
	return d->loader_mod_list;
}

std::shared_ptr<ModList> OneSixInstance::resourcePackList()
{
	I_D(OneSixInstance);
	if (!d->resource_pack_list)
	{
		d->resource_pack_list.reset(new ModList(resourcePacksDir()));
	}
	d->resource_pack_list->update();
	return d->resource_pack_list;
}

QDialog *OneSixInstance::createModEditDialog(QWidget *parent)
{
	return new OneSixModEditDialog(this, parent);
}

bool OneSixInstance::setIntendedVersionId(QString version)
{
	settings().set("IntendedVersion", version);
	setShouldUpdate(true);
	QFile::remove(PathCombine(instanceRoot(), "version.json"));
	clearVersion();
	return true;
}

QString OneSixInstance::intendedVersionId() const
{
	return settings().get("IntendedVersion").toString();
}

void OneSixInstance::setShouldUpdate(bool val)
{
	settings().set("ShouldUpdate", val);
}

bool OneSixInstance::shouldUpdate() const
{
	QVariant var = settings().get("ShouldUpdate");
	if (!var.isValid() || var.toBool() == false)
	{
		return intendedVersionId() != currentVersionId();
	}
	return true;
}

bool OneSixInstance::versionIsCustom()
{
	QDir patches(PathCombine(instanceRoot(), "patches/"));
	return (patches.exists() && patches.count() >= 0)
			|| QFile::exists(PathCombine(instanceRoot(), "custom.json"))
			|| QFile::exists(PathCombine(instanceRoot(), "user.json"));
}

QString OneSixInstance::currentVersionId() const
{
	return intendedVersionId();
}

bool OneSixInstance::reloadVersion(QWidget *widgetParent)
{
	I_D(OneSixInstance);

	bool ret = d->version->reload(widgetParent, false, externalPatches());
	if (ret)
	{
		ret = d->vanillaVersion->reload(widgetParent, true, externalPatches());
	}

	emit versionReloaded();
	return ret;
}

void OneSixInstance::clearVersion()
{
	I_D(OneSixInstance);
	d->version->clear();
	d->vanillaVersion->clear();
	emit versionReloaded();
}

std::shared_ptr<OneSixVersion> OneSixInstance::getFullVersion() const
{
	I_D(const OneSixInstance);
	return d->version;
}

std::shared_ptr<OneSixVersion> OneSixInstance::getVanillaVersion() const
{
	I_D(const OneSixInstance);
	return d->vanillaVersion;
}

QString OneSixInstance::defaultBaseJar() const
{
	return "versions/" + intendedVersionId() + "/" + intendedVersionId() + ".jar";
}

QString OneSixInstance::defaultCustomBaseJar() const
{
	return PathCombine(instanceRoot(), "custom.jar");
}

bool OneSixInstance::menuActionEnabled(QString action_name) const
{
	if (action_name == "actionChangeInstLWJGLVersion")
		return false;
	return true;
}

QString OneSixInstance::getStatusbarDescription()
{
	QString descr = "OneSix : " + intendedVersionId();
	if (versionIsCustom())
	{
		descr + " (custom)";
	}
	return descr;
}

QDir OneSixInstance::librariesPath() const
{
	return QDir::current().absoluteFilePath("libraries");
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

QString OneSixInstance::loaderModsDir() const
{
	return PathCombine(minecraftRoot(), "mods");
}

QString OneSixInstance::resourcePacksDir() const
{
	return PathCombine(minecraftRoot(), "resourcepacks");
}

QString OneSixInstance::instanceConfigFolder() const
{
	return PathCombine(minecraftRoot(), "config");
}
