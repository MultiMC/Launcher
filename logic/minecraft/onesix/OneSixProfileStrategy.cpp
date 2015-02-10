#include "minecraft/onesix/OneSixProfileStrategy.h"
#include "minecraft/VersionBuildError.h"
#include "minecraft/onesix/OneSixInstance.h"
#include "minecraft/onesix/OneSixFormat.h"
#include "minecraft/wonko/WonkoFormat.h"
#include "wonko/WonkoPackage.h"
#include "Env.h"
#include <Json.h>

#include <pathutils.h>
#include <QDir>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>

OneSixProfileStrategy::OneSixProfileStrategy(OneSixInstance *instance)
{
	m_instance = instance;
}

void OneSixProfileStrategy::upgradeDeprecatedFiles()
{
	auto versionJsonPath = PathCombine(m_instance->instanceRoot(), "version.json");
	auto customJsonPath = PathCombine(m_instance->instanceRoot(), "custom.json");
	auto mcJson = PathCombine(m_instance->instanceRoot(), "patches", "net.minecraft.json");

	// if custom.json exists
	if (QFile::exists(customJsonPath))
	{
		// can we create the patches folder to move it to?
		if (!ensureFilePathExists(mcJson))
		{
			throw VersionBuildError(QObject::tr("Unable to create path for %1").arg(mcJson));
		}
		// if version.json exists, remove it first
		if (QFile::exists(versionJsonPath))
		{
			if (!QFile::remove(versionJsonPath))
			{
				throw VersionBuildError(
					QObject::tr("Unable to remove obsolete %1").arg(versionJsonPath));
			}
		}
		// and then move the custom.json in place
		if (!QFile::rename(customJsonPath, mcJson))
		{
			throw VersionBuildError(
				QObject::tr("Unable to rename %1 to %2").arg(customJsonPath).arg(mcJson));
		}
	}
	// otherwise if version.json exists
	else if (QFile::exists(versionJsonPath))
	{
		// can we create the patches folder to move it to?
		if (!ensureFilePathExists(mcJson))
		{
			throw VersionBuildError(QObject::tr("Unable to create path for %1").arg(mcJson));
		}
		// and then move the custom.json in place
		if (!QFile::rename(versionJsonPath, mcJson))
		{
			throw VersionBuildError(
				QObject::tr("Unable to rename %1 to %2").arg(versionJsonPath).arg(mcJson));
		}
	}
}

void OneSixProfileStrategy::loadBuiltinPatch(QString uid, QString version)
{
	auto mc = std::dynamic_pointer_cast<WonkoPackage>(ENV.getVersionList(uid));
	auto path = mc->versionFilePath(version);
	if (!QFile::exists(path))
	{
		throw VersionIncomplete(uid);
	}
	QFile file(path);
	if (!file.open(QFile::ReadOnly))
	{
		throw Json::JsonException(QObject::tr("Unable to open the version file %1: %2.")
									.arg(file.fileName(), file.errorString()));
	}
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		throw Json::JsonException(
			QObject::tr("Unable to process the version file %1: %2 at %3.")
				.arg(file.fileName(), error.errorString())
				.arg(error.offset));
	}
	auto minecraftPatch = WonkoFormat::fromJson(doc, file.fileName());
	profile->appendPatch(minecraftPatch);
}

void OneSixProfileStrategy::loadLoosePatch(QString uid, QString name)
{
	auto mcJson = PathCombine(m_instance->instanceRoot(), "patches", QString("%1.json").arg(uid));
	auto file = ProfileUtils::parseJsonFile(QFileInfo(mcJson), false);
	file->fileId = uid;
	if(file->name.isEmpty())
	{
		file->name = name;
	}
	if (file->version.isEmpty())
	{
		file->version = QObject::tr("Custom");
	}
	profile->appendPatch(file);
}

void OneSixProfileStrategy::load()
{
	// upgrade old patch files, if any
	upgradeDeprecatedFiles();

	LoadOrder loadOrder;
	loadOrder.insert({"net.minecraft", m_instance->minecraftVersion()}, "hardcoded");
	loadOrder.insert({"org.lwjgl", m_instance->lwjglVersion()}, "hardcoded");

	// load all patches, put into map for ordering, apply in the right order
	ProfileUtils::PatchOrder userOrder;
	bool orderLoaded = ProfileUtils::readOverrideOrders(PathCombine(m_instance->instanceRoot(), "order.json"), userOrder);
	QDir patches(PathCombine(m_instance->instanceRoot(), "patches"));

	// HACK for the hardcoded forge and liteloader when order file is missing.
	if(!orderLoaded)
	{
		if(!m_instance->forgeVersion().isEmpty())
		{
			userOrder.push_back("net.minecraftforge");
		}
		if(!m_instance->liteloaderVersion().isEmpty())
		{
			userOrder.push_back("com.mumfrey.liteloader");
		}
	}

	// for each order item:
	// first, queue up things by sort order.
	for (auto id : userOrder)
	{
		// ignore hardcoded
		if(loadOrder.seen(id))
			continue;

		// parse the file
		QString filename = patches.absoluteFilePath(id + ".json");
		QFileInfo finfo(filename);
		// if the override file doesn't exist
		if (finfo.exists())
		{
			// it's override
			loadOrder.insert({id, "override"}, "override file by user order");
		}
		else
		{
			// not override... do we know the version?
			if(id == "net.minecraftforge")
			{
				if(!m_instance->forgeVersion().isEmpty())
				{
					loadOrder.insert({id, m_instance->forgeVersion()}, "forge by user order");
					continue;
				}
			}
			else if(id == "com.mumfrey.liteloader")
			{
				if(!m_instance->liteloaderVersion().isEmpty())
				{
					loadOrder.insert({id, m_instance->liteloaderVersion()}, "liteloader by user order");
					continue;
				}
			}
			qDebug() << "Patch file " << filename << " was deleted by external means...";
		}
	}

	// then load the rest of the 'loose' patches
	QMap<int, QStringList> files;
	QStringList unorderedFiles;
	for (auto info : patches.entryInfoList(QStringList() << "*.json", QDir::Files, QDir::Time))
	{
		// parse the file
		qDebug() << "Reading" << info.fileName();
		auto file = ProfileUtils::parseJsonFile(info, true);
		// ignore builtins
		if(loadOrder.seen(info.completeBaseName()))
			continue;
		// do not load what we already loaded in the first pass
		if (userOrder.contains(file->fileId))
			continue;
		int fileOrder = file->getOrder();
		if(fileOrder < 0)
		{
			unorderedFiles.push_back(info.completeBaseName());
		}
		else if (files.contains(fileOrder))
		{
			files[fileOrder].push_back(info.completeBaseName());
		}
		else
		{
			files.insert(fileOrder, {info.completeBaseName()});
		}
	}
	for (auto order : files.keys())
	{
		auto &filesList = files[order];
		for(auto item: filesList)
		{
			loadOrder.insert({item, "override"}, "override by internal order number");
		}
	}
	for (auto item: unorderedFiles)
	{
		loadOrder.insert({item, "override"}, "override by file timestamp");
	}

	// actually load the files.
	profile->clearPatches();
	for(auto item: loadOrder.loadOrder)
	{
		// if it's a loose override file:
		if(item.version == "override")
		{
			loadLoosePatch(item.uid, item.uid);
		}
		// if it's a wonko file:
		else
		{
			loadBuiltinPatch(item.uid, item.version);
		}
	}
	profile->resources.finalize();
}

bool OneSixProfileStrategy::saveOrder(ProfileUtils::PatchOrder order)
{
	return ProfileUtils::writeOverrideOrders(
		PathCombine(m_instance->instanceRoot(), "order.json"), order);
}

bool OneSixProfileStrategy::removePatch(PackagePtr patch)
{
	bool ok = true;
	// first, remove the patch file. this ensures it's not used anymore
	auto fileName = patch->getPatchFilename();
	if(fileName.size())
	{
		QFile patchFile(fileName);
		if(patchFile.exists() && !patchFile.remove())
		{
			qCritical() << "File" << fileName << "could not be removed because:" << patchFile.errorString();
			return false;
		}
	}

	auto preRemoveJarMod = [&](JarmodPtr jarMod) -> bool
	{
		QString fullpath = PathCombine(m_instance->jarModsDir(), jarMod->name);
		QFileInfo finfo(fullpath);
		if (finfo.exists())
		{
			QFile jarModFile(fullpath);
			if(!jarModFile.remove())
			{
				qCritical() << "File" << fullpath << "could not be removed because:" << jarModFile.errorString();
				return false;
			}
			return true;
		}
		return true;
	};

	for (auto &jarmod : patch->resources.jarMods)
	{
		ok &= preRemoveJarMod(jarmod);
	}
	return ok;
}

bool OneSixProfileStrategy::installJarMods(QStringList filepaths)
{
	QString patchDir = PathCombine(m_instance->instanceRoot(), "patches");
	if (!ensureFolderPathExists(patchDir))
	{
		return false;
	}

	if (!ensureFolderPathExists(m_instance->jarModsDir()))
	{
		return false;
	}

	for (auto filepath : filepaths)
	{
		QFileInfo sourceInfo(filepath);
		auto uuid = QUuid::createUuid();
		QString id = uuid.toString().remove('{').remove('}');
		QString target_filename = id + ".jar";
		QString target_id = "org.multimc.jarmod." + id;
		QString target_name = sourceInfo.completeBaseName() + " (jar mod)";
		QString finalPath = PathCombine(m_instance->jarModsDir(), target_filename);

		QFileInfo targetInfo(finalPath);
		if (targetInfo.exists())
		{
			return false;
		}

		if (!QFile::copy(sourceInfo.absoluteFilePath(),
						 QFileInfo(finalPath).absoluteFilePath()))
		{
			return false;
		}

		auto f = std::make_shared<Package>();
		auto jarMod = std::make_shared<Jarmod>();
		jarMod->name = target_filename;
		f->resources.jarMods.append(jarMod);
		f->name = target_name;
		f->fileId = target_id;
		QString patchFileName = PathCombine(patchDir, target_id + ".json");
		f->setPatchFilename(patchFileName);

		QSaveFile file(patchFileName);
		if (!file.open(QFile::WriteOnly))
		{
			qCritical() << "Error opening" << file.fileName()
						<< "for reading:" << file.errorString();
			return false;
		}
		file.write(OneSixFormat::toJson(f, false).toJson());
		file.commit();
		profile->appendPatch(f);
	}
	profile->saveCurrentOrder();
	profile->reapply();
	return true;
}
