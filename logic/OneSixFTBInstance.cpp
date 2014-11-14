#include "OneSixFTBInstance.h"

#include "logic/minecraft/InstanceVersion.h"
#include "logic/minecraft/OneSixLibrary.h"
#include "logic/minecraft/VersionBuilder.h"
#include "tasks/SequentialTask.h"
#include "forge/ForgeInstaller.h"
#include "forge/ForgeVersionList.h"
#include "OneSixInstance_p.h"
#include "MultiMC.h"
#include "pathutils.h"
#include "FTBUtils.h"

OneSixFTBInstance::OneSixFTBInstance(const QString &rootDir, SettingsObject *settings, QObject *parent) :
	OneSixInstance(rootDir, settings, parent)
{
	m_launchOptions = FTBUtils::getLaunchOptions();
}

void OneSixFTBInstance::copy(const QDir &newDir)
{
	QStringList libraryNames;
	// create patch file
	{
		QLOG_DEBUG() << "Creating patch file for FTB instance...";
		QFile f(minecraftRoot() + "/pack.json");
		if (!f.open(QFile::ReadOnly))
		{
			QLOG_ERROR() << "Couldn't open" << f.fileName() << ":" << f.errorString();
			return;
		}
		QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
		QJsonArray libs = root.value("libraries").toArray();
		QJsonArray outLibs;
		for (auto lib : libs)
		{
			QJsonObject libObj = lib.toObject();
			libObj.insert("MMC-hint", QString("local"));
			libObj.insert("insert", QString("prepend"));
			libraryNames.append(libObj.value("name").toString());
			outLibs.append(libObj);
		}
		root.remove("libraries");
		root.remove("id");
		root.insert("+libraries", outLibs);
		root.insert("order", 1);
		root.insert("fileId", QString("org.multimc.ftb.pack.json"));
		root.insert("name", name());
		root.insert("mcVersion", intendedVersionId());
		root.insert("version", intendedVersionId());
		ensureFilePathExists(newDir.absoluteFilePath("patches/ftb.json"));
		QFile out(newDir.absoluteFilePath("patches/ftb.json"));
		if (!out.open(QFile::WriteOnly | QFile::Truncate))
		{
			QLOG_ERROR() << "Couldn't open" << out.fileName() << ":" << out.errorString();
			return;
		}
		out.write(QJsonDocument(root).toJson());
	}
	// copy libraries
	{
		QLOG_DEBUG() << "Copying FTB libraries";
		for (auto library : libraryNames)
		{
			OneSixLibrary *lib = new OneSixLibrary(library);
			const QString out = QDir::current().absoluteFilePath("libraries/" + lib->storagePath());
			if (QFile::exists(out))
			{
				continue;
			}
			if (!ensureFilePathExists(out))
			{
				QLOG_ERROR() << "Couldn't create folder structure for" << out;
			}
			if (!QFile::copy(librariesPath().absoluteFilePath(lib->storagePath()), out))
			{
				QLOG_ERROR() << "Couldn't copy" << lib->rawName();
			}
		}
	}
}

QString OneSixFTBInstance::id() const
{
	return "FTB/" + BaseInstance::id();
}

QDir OneSixFTBInstance::librariesPath() const
{
	return QDir(MMC->settings()->get("FTBRoot").toString() + "/libraries");
}

QDir OneSixFTBInstance::versionsPath() const
{
	return QDir(MMC->settings()->get("FTBRoot").toString() + "/versions");
}

QStringList OneSixFTBInstance::externalPatches() const
{
	return QStringList() << versionsPath().absoluteFilePath(intendedVersionId() + "/" + intendedVersionId() + ".json")
						 << minecraftRoot() + "/pack.json";
}

bool OneSixFTBInstance::providesVersionFile() const
{
	return true;
}

int OneSixFTBInstance::defaultMaxMemory() const
{
	return m_launchOptions.ramMax.toInt();
}

QString OneSixFTBInstance::defaultJavaArgs() const
{
	return m_launchOptions.additionalJavaOptions;
}

void OneSixFTBInstance::postCopy(InstancePtr copy)
{
	/* if the original value is different from the value in the copy (=> default value was
	   changed using the ftblaunch.cfg file, but not modified by the user) we need to make the
	   FTB provided value permanent */
	if (copy->settings().get("OverrideJavaArgs").toBool() !=
		settings().get("OverrideJavaArgs").toBool())
	{
		copy->settings().set("OverrideJavaArgs", settings().get("OverrideJavaArgs").toBool());
	}
	if (copy->settings().get("JvmArgs").toString() != settings().get("JvmArgs").toString())
	{
		copy->settings().set("JvmArgs", settings().get("JvmArgs").toString());
	}
	if (copy->settings().get("MaxMemAlloc").toInt() != settings().get("MaxMemAlloc").toInt())
	{
		copy->settings().set("MaxMemAlloc", settings().get("MaxMemAlloc").toInt());
	}
}

QString OneSixFTBInstance::getStatusbarDescription()
{
	if (flags() & VersionBrokenFlag)
	{
		return "OneSix FTB: " + intendedVersionId() + " (broken)";
	}
	return "OneSix FTB: " + intendedVersionId();
}

std::shared_ptr<Task> OneSixFTBInstance::doUpdate()
{
	return OneSixInstance::doUpdate();
}

#include "OneSixFTBInstance.moc"
