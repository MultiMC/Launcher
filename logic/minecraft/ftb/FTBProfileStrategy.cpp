#include "minecraft/ftb/FTBProfileStrategy.h"
#include "minecraft/VersionBuildError.h"
#include "minecraft/ftb/FTBInstance.h"
#include <minecraft/VersionFilterData.h>

#include <pathutils.h>
#include <QDir>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>

FTBProfileStrategy::FTBProfileStrategy(FTBInstance* instance) : OneSixProfileStrategy(instance)
{
}

void FTBProfileStrategy::load()
{
	profile->clearPatches();
	loadBuiltinPatch("net.minecraft", m_instance->minecraftVersion());
	loadBuiltinPatch("org.lwjgl", m_instance->lwjglVersion());

	// FTB hack -> fake upgrade of libs.
	if(m_instance->minecraftVersion() == "1.7.10")
	{
		auto patch = profile->versionPatch("net.minecraft");
		for(auto lib: patch->resources.libraries->addLibs)
		{
			auto prefix = lib->name().artifactPrefix();
			if(prefix == "com.google.guava:guava")
			{
				lib->setName(GradleSpecifier("com.google.guava:guava:16.0"));
			}
			else if(prefix == "org.apache.commons:commons-lang3")
			{
				lib->setName(GradleSpecifier("org.apache.commons:commons-lang3:3.2.1"));
			}
		}
	}

	auto nativeInstance = dynamic_cast<FTBInstance *>(m_instance);
	// FTB hack - load the pack.json
	{
		PackagePtr ftbPackJson;
		auto mcJson = m_instance->minecraftRoot() + "/pack.json";
		auto patch = profile->versionPatch("net.minecraft");
		if (patch->m_releaseTime >= g_VersionFilterData.legacyCutoffDate && QFile::exists(mcJson))
		{
			ftbPackJson = ProfileUtils::parseJsonFile(QFileInfo(mcJson), false);

			// adapt the loaded file - the FTB patch file format is different than ours.
			auto & patch = ftbPackJson->resources;
			patch.libraries->addLibs = patch.libraries->overwriteLibs;
			patch.libraries->overwriteLibs.clear();
			patch.libraries->shouldOverwriteLibs = false;

			// FIXME: possibly broken, needs testing
			// file->id.clear();
			for(auto addLib: patch.libraries->addLibs)
			{
				addLib->m_hint = "local";
				addLib->insertType = Library::Prepend;
				addLib->setStoragePrefix(nativeInstance->FTBLibraryPrefix());
			}
		}
		else
		{
			qDebug() << "FTB instance" << m_instance->name() << "does not contain a pack.json, making a synthetic patch";
			ftbPackJson = std::make_shared<Package>();
		}

		// FTB hack - load all jar mods from instMods
		ModList jarMods(m_instance->jarModsDir());
		jarMods.update();
		for(auto mod : jarMods.allMods())
		{
			if(mod.type() == Mod::MOD_ZIPFILE)
			{
				auto jarmod = std::make_shared<Jarmod>();
				jarmod->name = mod.filename().fileName();
				ftbPackJson->resources.jarMods.append(jarmod);
			}
			else
			{
				qDebug() << "Unknown mod from instMods ignored:" << mod.filename().fileName();
			}
		}

		// common bits and pieces
		ftbPackJson->fileId = "org.multimc.ftb.pack";
		ftbPackJson->name = QObject::tr("%1 (FTB pack)").arg(m_instance->name());
		if(ftbPackJson->version.isEmpty())
		{
			ftbPackJson->version = QObject::tr("Unknown");
			QFile versionFile (PathCombine(m_instance->instanceRoot(), "version"));
			if(versionFile.exists())
			{
				if(versionFile.open(QIODevice::ReadOnly))
				{
					// FIXME: just guessing the encoding/charset here.
					auto version = QString::fromUtf8(versionFile.readAll());
					ftbPackJson->version = version;
				}
			}
		}
		ftbPackJson->setOrder(1);
		profile->appendPatch(ftbPackJson);
	}

	profile->resources.finalize();
}

bool FTBProfileStrategy::saveOrder(ProfileUtils::PatchOrder order)
{
	return false;
}

bool FTBProfileStrategy::removePatch(PackagePtr patch)
{
	return false;
}

bool FTBProfileStrategy::installJarMods(QStringList filepaths)
{
	return false;
}
