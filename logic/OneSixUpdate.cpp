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

#include "MultiMC.h"
#include "OneSixUpdate.h"

#include <QtNetwork>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDataStream>

#include "BaseInstance.h"
#include "lists/MinecraftVersionList.h"
#include "VersionFinal.h"
#include "OneSixLibrary.h"
#include "OneSixInstance.h"
#include "net/ForgeMirrors.h"
#include "net/URLConstants.h"
#include "assets/AssetsUtils.h"

#include "pathutils.h"
#include <JlCompress.h>

OneSixUpdate::OneSixUpdate(OneSixInstance *inst, QObject *parent)
	: Task(parent), m_inst(inst)
{
}

void OneSixUpdate::executeTask()
{
	QString intendedVersion = m_inst->intendedVersionId();

	// Make directories
	QDir mcDir(m_inst->minecraftRoot());
	if (!mcDir.exists() && !mcDir.mkpath("."))
	{
		emitFailed(tr("Failed to create folder for minecraft binaries."));
		return;
	}

	if (m_inst->shouldUpdate())
	{
		// Get a pointer to the version object that corresponds to the instance's version.
		targetVersion = std::dynamic_pointer_cast<MinecraftVersion>(
			MMC->minecraftlist()->findVersion(intendedVersion));
		if (targetVersion == nullptr)
		{
			// don't do anything if it was invalid
			emitFailed(tr("The specified Minecraft version is invalid. Choose a different one."));
			return;
		}
		versionFileStart();
	}
	else
	{
		jarlibStart();
	}
}

void OneSixUpdate::versionFileStart()
{
	if (m_inst->providesVersionFile())
	{
		jarlibStart();
		return;
	}
	QLOG_INFO() << m_inst->name() << ": getting version file.";
	setStatus(tr("Getting the version files from Mojang..."));

	QString urlstr = "http://" + URLConstants::AWS_DOWNLOAD_VERSIONS +
					 targetVersion->descriptor() + "/" + targetVersion->descriptor() + ".json";
	auto job = new NetJob("Version index");
	job->addNetAction(ByteArrayDownload::make(QUrl(urlstr)));
	specificVersionDownloadJob.reset(job);
	connect(specificVersionDownloadJob.get(), SIGNAL(succeeded()), SLOT(versionFileFinished()));
	connect(specificVersionDownloadJob.get(), SIGNAL(failed()), SLOT(versionFileFailed()));
	connect(specificVersionDownloadJob.get(), SIGNAL(progress(qint64, qint64)),
			SIGNAL(progress(qint64, qint64)));
	specificVersionDownloadJob->start();
}

void OneSixUpdate::versionFileFinished()
{
	NetActionPtr DlJob = specificVersionDownloadJob->first();

	QString version_id = targetVersion->descriptor();
	QString inst_dir = m_inst->instanceRoot();
	// save the version file in $instanceId/version.json
	{
		QString version1 = PathCombine(inst_dir, "/version.json");
		ensureFilePathExists(version1);
		// FIXME: detect errors here, download to a temp file, swap
		QSaveFile vfile1(version1);
		if (!vfile1.open(QIODevice::Truncate | QIODevice::WriteOnly))
		{
			emitFailed(tr("Can't open %1 for writing.").arg(version1));
			return;
		}
		auto data = std::dynamic_pointer_cast<ByteArrayDownload>(DlJob)->m_data;

		// because mojang are screwing things up for april fools...
		{
			const QString assetsStartString = "\"assets\": \"";
			const int assetsStartIndex =
				data.indexOf(assetsStartString) + assetsStartString.size();
			const int assetsEndIndex = data.indexOf("\",", assetsStartIndex);
			const QString assetsType =
				data.mid(assetsStartIndex, assetsEndIndex - assetsStartIndex);
			if (assetsType.endsWith("_af"))
			{
				QLOG_INFO() << "April fools detected";
				const int afStart = data.indexOf("_af", assetsStartIndex);
				data = data.remove(afStart, assetsEndIndex - afStart);
				const QString afFileName =
					PathCombine(inst_dir, "/patches/net.minecraft.aprilfools.2014.json");
				ensureFilePathExists(afFileName);
				QSaveFile afFile(afFileName);
				if (afFile.open(QSaveFile::Truncate | QSaveFile::WriteOnly))
				{
					afFile.write(QString("{\"fileId\":\"net.minecraft.aprilfools.2014\","
										 "\"name\":\"Mojang April Fools 2014\",\"order\": "
										 "500,\"version\": \"\",\"assets\":\"" +
										 assetsType + "\"}").toLatin1());
				}
				if (!afFile.commit())
				{
					QLOG_ERROR() << "Couldn't write april fools file:" << afFile.errorString();
				}
			}
		}

		qint64 actual = 0;
		if ((actual = vfile1.write(data)) != data.size())
		{
			emitFailed(tr("Failed to write into %1. Written %2 out of %3.").arg(version1).arg(actual).arg(data.size()));
			return;
		}
		if (!vfile1.commit())
		{
			emitFailed(tr("Can't commit changes to %1").arg(version1));
			return;
		}
	}

	// the version is downloaded safely. update is 'done' at this point
	m_inst->setShouldUpdate(false);

	// delete any custom version inside the instance (it's no longer relevant, we did an update)
	QString custom = PathCombine(inst_dir, "/custom.json");
	QFile finfo(custom);
	if (finfo.exists())
	{
		finfo.remove();
	}
	// NOTE: Version is reloaded in jarlibStart
	jarlibStart();
}

void OneSixUpdate::versionFileFailed()
{
	emitFailed(tr("Failed to download the version description. Try again."));
}

void OneSixUpdate::assetIndexStart()
{
	setStatus(tr("Updating assets index..."));
	OneSixInstance *inst = (OneSixInstance *)m_inst;
	std::shared_ptr<VersionFinal> version = inst->getFullVersion();
	QString assetName = version->assets;
	QUrl indexUrl = "http://" + URLConstants::AWS_DOWNLOAD_INDEXES + assetName + ".json";
	QString localPath = assetName + ".json";
	auto job = new NetJob(tr("Asset index for %1").arg(inst->name()));

	auto metacache = MMC->metacache();
	auto entry = metacache->resolveEntry("asset_indexes", localPath);
	job->addNetAction(CacheDownload::make(indexUrl, entry));
	jarlibDownloadJob.reset(job);

	connect(jarlibDownloadJob.get(), SIGNAL(succeeded()), SLOT(assetIndexFinished()));
	connect(jarlibDownloadJob.get(), SIGNAL(failed()), SLOT(assetIndexFailed()));
	connect(jarlibDownloadJob.get(), SIGNAL(progress(qint64, qint64)),
			SIGNAL(progress(qint64, qint64)));

	jarlibDownloadJob->start();
}

void OneSixUpdate::assetIndexFinished()
{
	AssetsIndex index;

	OneSixInstance *inst = (OneSixInstance *)m_inst;
	std::shared_ptr<VersionFinal> version = inst->getFullVersion();
	QString assetName = version->assets;

	QString asset_fname = "assets/indexes/" + assetName + ".json";
	if (!AssetsUtils::loadAssetsIndexJson(asset_fname, &index))
	{
		emitFailed(tr("Failed to read the assets index!"));
	}

	QList<Md5EtagDownloadPtr> dls;
	for (auto object : index.objects.values())
	{
		QString objectName = object.hash.left(2) + "/" + object.hash;
		QFileInfo objectFile("assets/objects/" + objectName);
		if ((!objectFile.isFile()) || (objectFile.size() != object.size))
		{
			auto objectDL = MD5EtagDownload::make(
				QUrl("http://" + URLConstants::RESOURCE_BASE + objectName),
				objectFile.filePath());
			objectDL->m_total_progress = object.size;
			dls.append(objectDL);
		}
	}
	if (dls.size())
	{
		setStatus(tr("Getting the assets files from Mojang..."));
		auto job = new NetJob(tr("Assets for %1").arg(inst->name()));
		for (auto dl : dls)
			job->addNetAction(dl);
		jarlibDownloadJob.reset(job);
		connect(jarlibDownloadJob.get(), SIGNAL(succeeded()), SLOT(assetsFinished()));
		connect(jarlibDownloadJob.get(), SIGNAL(failed()), SLOT(assetsFailed()));
		connect(jarlibDownloadJob.get(), SIGNAL(progress(qint64, qint64)),
				SIGNAL(progress(qint64, qint64)));
		jarlibDownloadJob->start();
		return;
	}
	assetsFinished();
}

void OneSixUpdate::assetIndexFailed()
{
	emitFailed(tr("Failed to download the assets index!"));
}

void OneSixUpdate::assetsFinished()
{
	emitSucceeded();
}

void OneSixUpdate::assetsFailed()
{
	emitFailed(tr("Failed to download assets!"));
}

void OneSixUpdate::jarlibStart()
{
	setStatus(tr("Getting the library files from Mojang..."));
	QLOG_INFO() << m_inst->name() << ": downloading libraries";
	OneSixInstance *inst = (OneSixInstance *)m_inst;
	try
	{
		inst->reloadVersion();
	}
	catch(MMCError & e)
	{
		emitFailed(e.cause());
		return;
	}
	catch(...)
	{
		emitFailed(tr("Failed to load the version description file for reasons unknown."));
		return;
	}

	// Build a list of URLs that will need to be downloaded.
	std::shared_ptr<VersionFinal> version = inst->getFullVersion();
	// minecraft.jar for this version
	{
		QString version_id = version->id;
		QString localPath = version_id + "/" + version_id + ".jar";
		QString urlstr = "http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + localPath;

		auto job = new NetJob(tr("Libraries for instance %1").arg(inst->name()));

		auto metacache = MMC->metacache();
		auto entry = metacache->resolveEntry("versions", localPath);
		job->addNetAction(CacheDownload::make(QUrl(urlstr), entry));

		jarlibDownloadJob.reset(job);
	}

	auto libs = version->getActiveNativeLibs();
	libs.append(version->getActiveNormalLibs());

	auto metacache = MMC->metacache();
	QList<ForgeXzDownloadPtr> ForgeLibs;
	for (auto lib : libs)
	{
		if (lib->hint() == "local")
			continue;

		QString raw_storage = lib->storagePath();
		QString raw_dl = lib->downloadUrl();

		auto f = [&](QString storage, QString dl)
		{
			auto entry = metacache->resolveEntry("libraries", storage);
			if (entry->stale)
			{
				if (lib->hint() == "forge-pack-xz")
				{
					ForgeLibs.append(ForgeXzDownload::make(storage, entry));
				}
				else
				{
					jarlibDownloadJob->addNetAction(CacheDownload::make(dl, entry));
				}
			}
		};
		if (raw_storage.contains("${arch}"))
		{
			QString cooked_storage = raw_storage;
			QString cooked_dl = raw_dl;
			f(cooked_storage.replace("${arch}", "32"), cooked_dl.replace("${arch}", "32"));
			cooked_storage = raw_storage;
			cooked_dl = raw_dl;
			f(cooked_storage.replace("${arch}", "64"), cooked_dl.replace("${arch}", "64"));
		}
		else
		{
			f(raw_storage, raw_dl);
		}
	}
	// TODO: think about how to propagate this from the original json file... or IF AT ALL
	QString forgeMirrorList = "http://files.minecraftforge.net/mirror-brand.list";
	if (!ForgeLibs.empty())
	{
		jarlibDownloadJob->addNetAction(
			ForgeMirrors::make(ForgeLibs, jarlibDownloadJob, forgeMirrorList));
	}

	connect(jarlibDownloadJob.get(), SIGNAL(succeeded()), SLOT(jarlibFinished()));
	connect(jarlibDownloadJob.get(), SIGNAL(failed()), SLOT(jarlibFailed()));
	connect(jarlibDownloadJob.get(), SIGNAL(progress(qint64, qint64)),
			SIGNAL(progress(qint64, qint64)));

	jarlibDownloadJob->start();
}

void OneSixUpdate::jarlibFinished()
{
	assetIndexStart();
}

void OneSixUpdate::jarlibFailed()
{
	QStringList failed = jarlibDownloadJob->getFailedFiles();
	QString failed_all = failed.join("\n");
	emitFailed(tr("Failed to download the following files:\n%1\n\nPlease try again.").arg(failed_all));
}
