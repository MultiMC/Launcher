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

#include "OptiFineInstaller.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "logger/QsLog.h"

#include "VersionFinal.h"
#include "OneSixLibrary.h"
#include "OneSixInstance.h"
#include "MultiMC.h"
#include "pathutils.h"

QMap<QString, QString> OptiFineInstaller::m_mcVersionLaunchWrapperVersionMapping;

OptiFineInstaller::OptiFineInstaller() : BaseInstaller()
{
	if (m_mcVersionLaunchWrapperVersionMapping.isEmpty())
	{
		m_mcVersionLaunchWrapperVersionMapping["1.6.4"] = "1.6";
		m_mcVersionLaunchWrapperVersionMapping["1.7.2"] = "1.8";
		m_mcVersionLaunchWrapperVersionMapping["1.7.4"] = "1.9";
		m_mcVersionLaunchWrapperVersionMapping["1.7.5"] = "1.9";
	}
}

QFileInfo OptiFineInstaller::fileForVersion(const QString &version) const
{
	return QFileInfo(QDir::current().absoluteFilePath("libraries/optifine/OptiFine/" + version),
					 "OptiFine-" + version + ".jar");
}
QStringList OptiFineInstaller::getExistingVersions() const
{
	return QDir(QDir::current().absoluteFilePath("libraries/optifine/OptiFine"))
		.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

void OptiFineInstaller::aboutToInstallOther(std::shared_ptr<BaseInstaller> other,
											OneSixInstance *instance)
{
	if (other->id() != "net.minecraftforge" && other->id() != "com.mumfrey.liteloader")
	{
		return;
	}
	if (VersionFilePtr file = instance->getFullVersion()->versionFile(id()))
	{
		QString optifineLib;
		for (auto lib : file->addLibs)
		{
			if (lib->name.startsWith("optifine:OptiFine:"))
			{
				optifineLib = QDir::current().absoluteFilePath(
					PathCombine("libraries", VersionFile::createLibrary(lib)->storagePath()));
			}
		}
		if (optifineLib.isNull())
		{
			return;
		}
		const QString optifineModsLib = QDir::current().absoluteFilePath(
			PathCombine(instance->minecraftRoot(), "mods", QFileInfo(optifineLib).fileName()));
		if (!ensureFilePathExists(optifineModsLib))
		{
			throw MMCError(QObject::tr("Unable to create mods/ directory"));
		}
		QLOG_INFO() << "Attempting to copy" << optifineLib << "to" << optifineModsLib;
		if (!QFile::copy(optifineLib, optifineModsLib))
		{
			throw MMCError(QObject::tr("Unable to copy OptiFine to mods/"));
		}
		if (!QFile::remove(file->filename))
		{
			throw MMCError(QObject::tr("Unable to remove OptiFine patch"));
		}
	}
}

void OptiFineInstaller::prepare(OptiFineVersionPtr version)
{
	m_version = version;
}

bool OptiFineInstaller::add(OneSixInstance *to)
{
	if (!BaseInstaller::add(to))
	{
		return false;
	}

	QJsonObject obj;

	obj.insert("mainClass", QString("net.minecraft.launchwrapper.Launch"));
	obj.insert("+tweakers",
			   QJsonArray::fromStringList(QStringList() << "optifine.OptiFineTweaker"));
	obj.insert("order", 15);

	QJsonArray libraries;

	// launchwrapper
	{
		QJsonObject lwLib;
		lwLib.insert("name",
					 "net.minecraft:launchwrapper:" +
						 m_mcVersionLaunchWrapperVersionMapping[to->intendedVersionId()]);
		lwLib.insert("insert", QString("prepend"));
		libraries.append(lwLib);
	}
	// optifine
	{
		QJsonObject ofLib;
		ofLib.insert("name", "optifine:OptiFine:" + m_version->fullVersionString());
		ofLib.insert("MMC-hint", QString("local"));
		ofLib.insert("MMC-depend", QString("hard"));
		ofLib.insert("insert", QString("prepend"));
		libraries.append(ofLib);
	}

	obj.insert("+libraries", libraries);
	obj.insert("name", QString("OptiFine").remove('_'));
	obj.insert("fileId", id());
	obj.insert("version", m_version->versionString());
	obj.insert("mcVersion", to->intendedVersionId());

	QFile file(filename(to->instanceRoot()));
	if (!file.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Error opening" << file.fileName()
					 << "for reading:" << file.errorString();
		return false;
	}
	file.write(QJsonDocument(obj).toJson());
	file.close();

	return true;
}

bool OptiFineInstaller::canApply(const OneSixInstance *to)
{
	if (to->getFullVersion()->versionFile("net.minecraftforge") ||
		to->getFullVersion()->versionFile("com.mumfrey.liteloader"))
	{
		throw MMCError(QObject::tr("MinecraftForge or LiteLoader installed. You need to remove "
								   "those before installing OptiFine."));
	}
	return m_mcVersionLaunchWrapperVersionMapping.contains(to->intendedVersionId());
}

class OptiFineInstallTask : public Task
{
	Q_OBJECT
public:
	OptiFineInstallTask(OptiFineInstaller *installer, OneSixInstance *instance,
						BaseVersionPtr version, QObject *parent)
		: Task(parent), m_installer(installer), m_instance(instance), m_version(version)
	{
	}

protected:
	void executeTask() override
	{
		OptiFineVersionPtr optifineVersion =
			std::dynamic_pointer_cast<OptiFineVersion>(m_version);
		if (!optifineVersion)
		{
			return;
		}
		setStatus(tr("Copying optifine..."));
		const QString destFilename = QDir::current().absoluteFilePath(
			"libraries/optifine/OptiFine/" + optifineVersion->fullVersionString() +
			"/OptiFine-" + optifineVersion->fullVersionString() + ".jar");
		if (!QFile::exists(destFilename))
		{
			if (!ensureFilePathExists(destFilename))
			{
				emitFailed(tr("Couldn't create library directory structure for OptiFine"));
				return;
			}
			QFile f(optifineVersion->file().absoluteFilePath());
			if (!f.copy(destFilename))
			{
				emitFailed(
					tr("Couldn't copy OptiFine to it's destination: %1").arg(f.errorString()));
				return;
			}
		}
		m_installer->prepare(optifineVersion);
		if (!m_installer->add(m_instance))
		{
			emitFailed(tr("For reasons unknown, the OptiFine installation failed. Check your "
						  "MultiMC log files for details."));
		}
		else
		{
			try
			{
				m_instance->reloadVersion();
				emitSucceeded();
			}
			catch (MMCError &e)
			{
				emitFailed(e.cause());
			}
			catch (...)
			{
				emitFailed(
					tr("Failed to load the version description file for reasons unknown."));
			}
		}
	}

private:
	OptiFineInstaller *m_installer;
	OneSixInstance *m_instance;
	BaseVersionPtr m_version;
};

ProgressProvider *OptiFineInstaller::createInstallTask(OneSixInstance *instance,
													   BaseVersionPtr version, QObject *parent)
{
	return new OptiFineInstallTask(this, instance, version, parent);
}

#include "OptiFineInstaller.moc"
