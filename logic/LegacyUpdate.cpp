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

#include <QStringList>

#include <pathutils.h>
#include <quazip.h>
#include <quazipfile.h>
#include <JlCompress.h>

#include "logic/LegacyUpdate.h"
#include "logic/LwjglVersionList.h"
#include "logic/minecraft/MinecraftVersionList.h"
#include "logic/BaseInstance.h"
#include "logic/LegacyInstance.h"
#include "MultiMC.h"
#include "logic/ModList.h"

#include "logger/QsLog.h"
#include "logic/net/URLConstants.h"
#include "JarUtils.h"


LegacyUpdate::LegacyUpdate(BaseInstance *inst, QObject *parent) : Task(parent), m_inst(inst)
{
}

void LegacyUpdate::executeTask()
{
	fmllibsStart();
}

void LegacyUpdate::fmllibsStart()
{
	// Get the mod list
	LegacyInstance *inst = (LegacyInstance *)m_inst;
	auto modList = inst->jarModList();

	bool forge_present = false;

	QString version = inst->intendedVersionId();
	auto & fmlLibsMapping = g_VersionFilterData.fmlLibsMapping;
	if (!fmlLibsMapping.contains(version))
	{
		lwjglStart();
		return;
	}

	auto &libList = fmlLibsMapping[version];

	// determine if we need some libs for FML or forge
	setStatus(tr("Checking for FML libraries..."));
	for (unsigned i = 0; i < modList->size(); i++)
	{
		auto &mod = modList->operator[](i);

		// do not use disabled mods.
		if (!mod.enabled())
			continue;

		if (mod.type() != Mod::MOD_ZIPFILE)
			continue;

		if (mod.mmc_id().contains("forge", Qt::CaseInsensitive))
		{
			forge_present = true;
			break;
		}
		if (mod.mmc_id().contains("fml", Qt::CaseInsensitive))
		{
			forge_present = true;
			break;
		}
	}
	// we don't...
	if (!forge_present)
	{
		lwjglStart();
		return;
	}

	// now check the lib folder inside the instance for files.
	for (auto &lib : libList)
	{
		QFileInfo libInfo(PathCombine(inst->libDir(), lib.filename));
		if (libInfo.exists())
			continue;
		fmlLibsToProcess.append(lib);
	}

	// if everything is in place, there's nothing to do here...
	if (fmlLibsToProcess.isEmpty())
	{
		lwjglStart();
		return;
	}

	// download missing libs to our place
	setStatus(tr("Dowloading FML libraries..."));
	auto dljob = new NetJob("FML libraries");
	auto metacache = MMC->metacache();
	for (auto &lib : fmlLibsToProcess)
	{
		auto entry = metacache->resolveEntry("fmllibs", lib.filename);
		QString urlString = lib.ours ? URLConstants::FMLLIBS_OUR_BASE_URL + lib.filename
									 : URLConstants::FMLLIBS_FORGE_BASE_URL + lib.filename;
		dljob->addNetAction(CacheDownload::make(QUrl(urlString), entry));
	}

	connect(dljob, SIGNAL(succeeded()), SLOT(fmllibsFinished()));
	connect(dljob, SIGNAL(failed()), SLOT(fmllibsFailed()));
	connect(dljob, SIGNAL(progress(qint64, qint64)), SIGNAL(progress(qint64, qint64)));
	legacyDownloadJob.reset(dljob);
	legacyDownloadJob->start();
}

void LegacyUpdate::fmllibsFinished()
{
	legacyDownloadJob.reset();
	if(!fmlLibsToProcess.isEmpty())
	{
		setStatus(tr("Copying FML libraries into the instance..."));
		LegacyInstance *inst = (LegacyInstance *)m_inst;
		auto metacache = MMC->metacache();
		int index = 0;
		for (auto &lib : fmlLibsToProcess)
		{
			progress(index, fmlLibsToProcess.size());
			auto entry = metacache->resolveEntry("fmllibs", lib.filename);
			auto path = PathCombine(inst->libDir(), lib.filename);
			if(!ensureFilePathExists(path))
			{
				emitFailed(tr("Failed creating FML library folder inside the instance."));
				return;
			}
			if (!QFile::copy(entry->getFullPath(), PathCombine(inst->libDir(), lib.filename)))
			{
				emitFailed(tr("Failed copying Forge/FML library: %1.").arg(lib.filename));
				return;
			}
			index++;
		}
		progress(index, fmlLibsToProcess.size());
	}
	lwjglStart();
}

void LegacyUpdate::fmllibsFailed()
{
	emitFailed("Game update failed: it was impossible to fetch the required FML libraries.");
	return;
}

void LegacyUpdate::lwjglStart()
{
	LegacyInstance *inst = (LegacyInstance *)m_inst;

	lwjglVersion = inst->lwjglVersion();
	lwjglTargetPath = PathCombine(MMC->settings()->get("LWJGLDir").toString(), lwjglVersion);
	lwjglNativesPath = PathCombine(lwjglTargetPath, "natives");

	// if the 'done' file exists, we don't have to download this again
	QFileInfo doneFile(PathCombine(lwjglTargetPath, "done"));
	if (doneFile.exists())
	{
		jarStart();
		return;
	}

	auto list = MMC->lwjgllist();
	if (!list->isLoaded())
	{
		emitFailed("Too soon! Let the LWJGL list load :)");
		return;
	}

	setStatus(tr("Downloading new LWJGL..."));
	auto version = list->getVersion(lwjglVersion);
	if (!version)
	{
		emitFailed("Game update failed: the selected LWJGL version is invalid.");
		return;
	}

	QString url = version->url();
	QUrl realUrl(url);
	QString hostname = realUrl.host();
	auto worker = MMC->qnam();
	QNetworkRequest req(realUrl);
	req.setRawHeader("Host", hostname.toLatin1());
	req.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Cached)");
	QNetworkReply *rep = worker->get(req);

	m_reply = std::shared_ptr<QNetworkReply>(rep);
	connect(rep, SIGNAL(downloadProgress(qint64, qint64)), SIGNAL(progress(qint64, qint64)));
	connect(worker.get(), SIGNAL(finished(QNetworkReply *)),
			SLOT(lwjglFinished(QNetworkReply *)));
}

void LegacyUpdate::lwjglFinished(QNetworkReply *reply)
{
	if (m_reply.get() != reply)
	{
		return;
	}
	if (reply->error() != QNetworkReply::NoError)
	{
		emitFailed("Failed to download: " + reply->errorString() +
				   "\nSometimes you have to wait a bit if you download many LWJGL versions in "
				   "a row. YMMV");
		return;
	}
	auto worker = MMC->qnam();
	// Here i check if there is a cookie for me in the reply and extract it
	QList<QNetworkCookie> cookies =
		qvariant_cast<QList<QNetworkCookie>>(reply->header(QNetworkRequest::SetCookieHeader));
	if (cookies.count() != 0)
	{
		// you must tell which cookie goes with which url
		worker->cookieJar()->setCookiesFromUrl(cookies, QUrl("sourceforge.net"));
	}

	// here you can check for the 302 or whatever other header i need
	QVariant newLoc = reply->header(QNetworkRequest::LocationHeader);
	if (newLoc.isValid())
	{
		QString redirectedTo = reply->header(QNetworkRequest::LocationHeader).toString();
		QUrl realUrl(redirectedTo);
		QString hostname = realUrl.host();
		QNetworkRequest req(redirectedTo);
		req.setRawHeader("Host", hostname.toLatin1());
		req.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Cached)");
		QNetworkReply *rep = worker->get(req);
		connect(rep, SIGNAL(downloadProgress(qint64, qint64)),
				SIGNAL(progress(qint64, qint64)));
		m_reply = std::shared_ptr<QNetworkReply>(rep);
		return;
	}
	QFile saveMe("lwjgl.zip");
	saveMe.open(QIODevice::WriteOnly);
	saveMe.write(m_reply->readAll());
	saveMe.close();
	setStatus(tr("Installing new LWJGL..."));
	extractLwjgl();
	jarStart();
}
void LegacyUpdate::extractLwjgl()
{
	// make sure the directories are there

	bool success = ensureFolderPathExists(lwjglNativesPath);

	if (!success)
	{
		emitFailed("Failed to extract the lwjgl libs - error when creating required folders.");
		return;
	}

	QuaZip zip("lwjgl.zip");
	if (!zip.open(QuaZip::mdUnzip))
	{
		emitFailed("Failed to extract the lwjgl libs - not a valid archive.");
		return;
	}

	// and now we are going to access files inside it
	QuaZipFile file(&zip);
	const QString jarNames[] = {"jinput.jar", "lwjgl_util.jar", "lwjgl.jar"};
	for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile())
	{
		if (!file.open(QIODevice::ReadOnly))
		{
			zip.close();
			emitFailed("Failed to extract the lwjgl libs - error while reading archive.");
			return;
		}
		QuaZipFileInfo info;
		QString name = file.getActualFileName();
		if (name.endsWith('/'))
		{
			file.close();
			continue;
		}
		QString destFileName;
		// Look for the jars
		for (int i = 0; i < 3; i++)
		{
			if (name.endsWith(jarNames[i]))
			{
				destFileName = PathCombine(lwjglTargetPath, jarNames[i]);
			}
		}
		// Not found? look for the natives
		if (destFileName.isEmpty())
		{
#ifdef Q_OS_WIN32
			QString nativesDir = "windows";
#else
#ifdef Q_OS_MAC
			QString nativesDir = "macosx";
#else
			QString nativesDir = "linux";
#endif
#endif
			if (name.contains(nativesDir))
			{
				int lastSlash = name.lastIndexOf('/');
				int lastBackSlash = name.lastIndexOf('\\');
				if (lastSlash != -1)
					name = name.mid(lastSlash + 1);
				else if (lastBackSlash != -1)
					name = name.mid(lastBackSlash + 1);
				destFileName = PathCombine(lwjglNativesPath, name);
			}
		}
		// Now if destFileName is still empty, go to the next file.
		if (!destFileName.isEmpty())
		{
			setStatus(tr("Installing new LWJGL - extracting ") + name + "...");
			QFile output(destFileName);
			output.open(QIODevice::WriteOnly);
			output.write(file.readAll()); // FIXME: wste of memory!?
			output.close();
		}
		file.close(); // do not forget to close!
	}
	zip.close();
	m_reply.reset();
	QFile doneFile(PathCombine(lwjglTargetPath, "done"));
	doneFile.open(QIODevice::WriteOnly);
	doneFile.write("done.");
	doneFile.close();
}

void LegacyUpdate::lwjglFailed()
{
	emitFailed("Bad stuff happened while trying to get the lwjgl libs...");
}

void LegacyUpdate::jarStart()
{
	LegacyInstance *inst = (LegacyInstance *)m_inst;
	if (!inst->shouldUpdate() || inst->shouldUseCustomBaseJar())
	{
		ModTheJar();
		return;
	}

	setStatus(tr("Checking for jar updates..."));
	// Make directories
	QDir binDir(inst->binDir());
	if (!binDir.exists() && !binDir.mkpath("."))
	{
		emitFailed("Failed to create bin folder.");
		return;
	}

	// Build a list of URLs that will need to be downloaded.
	setStatus(tr("Downloading new minecraft.jar ..."));

	QString version_id = inst->intendedVersionId();
	QString localPath = version_id + "/" + version_id + ".jar";
	QString urlstr = "http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + localPath;

	auto dljob = new NetJob("Minecraft.jar for version " + version_id);

	auto metacache = MMC->metacache();
	auto entry = metacache->resolveEntry("versions", localPath);
	dljob->addNetAction(CacheDownload::make(QUrl(urlstr), entry));
	connect(dljob, SIGNAL(succeeded()), SLOT(jarFinished()));
	connect(dljob, SIGNAL(failed()), SLOT(jarFailed()));
	connect(dljob, SIGNAL(progress(qint64, qint64)), SIGNAL(progress(qint64, qint64)));
	legacyDownloadJob.reset(dljob);
	legacyDownloadJob->start();
}

void LegacyUpdate::jarFinished()
{
	// process the jar
	ModTheJar();
}

void LegacyUpdate::jarFailed()
{
	// bad, bad
	emitFailed("Failed to download the minecraft jar. Try again later.");
}

void LegacyUpdate::ModTheJar()
{
	LegacyInstance *inst = (LegacyInstance *)m_inst;

	if (!inst->shouldRebuild())
	{
		emitSucceeded();
		return;
	}

	// Get the mod list
	auto modList = inst->jarModList();

	QFileInfo runnableJar(inst->runnableJar());
	QFileInfo baseJar(inst->baseJar());
	bool base_is_custom = inst->shouldUseCustomBaseJar();

	// Nothing to do if there are no jar mods to install, no backup and just the mc jar
	if (base_is_custom)
	{
		// yes, this can happen if the instance only has the runnable jar and not the base jar
		// it *could* be assumed that such an instance is vanilla, but that wouldn't be safe
		// because that's not something mmc4 guarantees
		if (runnableJar.isFile() && !baseJar.exists() && modList->empty())
		{
			inst->setShouldRebuild(false);
			emitSucceeded();
			return;
		}

		setStatus(tr("Installing mods: Backing up minecraft.jar ..."));
		if (!baseJar.exists() && !QFile::copy(runnableJar.filePath(), baseJar.filePath()))
		{
			emitFailed("It seems both the active and base jar are gone. A fresh base jar will "
					   "be used on next run.");
			inst->setShouldRebuild(true);
			inst->setShouldUpdate(true);
			inst->setShouldUseCustomBaseJar(false);
			return;
		}
	}

	if (!baseJar.exists())
	{
		emitFailed("The base jar " + baseJar.filePath() + " does not exist");
		return;
	}

	if (runnableJar.exists() && !QFile::remove(runnableJar.filePath()))
	{
		emitFailed("Failed to delete old minecraft.jar");
		return;
	}

	// TaskStep(); // STEP 1
	setStatus(tr("Installing mods: Opening minecraft.jar ..."));

	const auto & mods = modList->allMods();
	QString outputJarPath = runnableJar.filePath();
	QString inputJarPath = baseJar.filePath();

	if(!JarUtils::createModdedJar(inputJarPath, outputJarPath, mods))
	{
		emitFailed(tr("Failed to create the custom Minecraft jar file."));
		return;
	}
	inst->setShouldRebuild(false);
	// inst->UpdateVersion(true);
	emitSucceeded();
	return;
}
