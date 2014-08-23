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

#include "QuickModFilesUpdater.h"

#include <QFile>
#include <QTimer>
#include <QDebug>
#include <memory>

#include <QJsonDocument>
#include <QJsonObject>

#include "logic/quickmod/QuickModsList.h"
#include "logic/quickmod/QuickMod.h"
#include "logic/net/ByteArrayDownload.h"
#include "logic/net/NetJob.h"
#include "logic/Mod.h"
#include "logic/MMCJson.h"
#include "MultiMC.h"
#include "modutils.h"
#include "pathutils.h"

#include "logger/QsLog.h"

QuickModFilesUpdater::QuickModFilesUpdater(QuickModsList *list) : QObject(list), m_list(list)
{
	m_quickmodDir = QDir::current();
	m_quickmodDir.mkdir("quickmod");
	m_quickmodDir.cd("quickmod");

	QTimer::singleShot(1, this, SLOT(readModFiles()));
}

void QuickModFilesUpdater::registerFile(const QUrl &url, bool sandbox)
{
	auto job = new NetJob("QuickMod download");
	auto download =
			ByteArrayDownload::make(url);
	download->m_followRedirects = true;
	download->setProperty("sandbox", sandbox);
	connect(download.get(), SIGNAL(succeeded(int)), this, SLOT(receivedMod(int)));
	connect(download.get(), SIGNAL(failed(int)), this, SLOT(failedMod(int)));
	job->addNetAction(download);
	job->start();
}

void QuickModFilesUpdater::unregisterMod(const QuickModPtr mod)
{
	m_quickmodDir.remove(fileName(mod));
}

void QuickModFilesUpdater::releaseFromSandbox(QuickModPtr mod)
{
	QFile::rename(m_quickmodDir.absoluteFilePath("sandbox/" + fileName(mod)),
				  m_quickmodDir.absoluteFilePath(fileName(mod)));
	m_list->addMod(mod);
}

void QuickModFilesUpdater::cleanupSandboxedFiles()
{
	QDir dir(m_quickmodDir.absoluteFilePath("sandbox"));
	for (auto entry : dir.entryInfoList(QDir::Files))
	{
		dir.remove(entry.absoluteFilePath());
	}
}

void QuickModFilesUpdater::update()
{
	auto job = new NetJob("QuickMod download");
	for (int i = 0; i < m_list->numMods(); ++i)
	{
		auto url = m_list->modAt(i)->updateUrl();
		if (url.isValid())
		{
			auto download =
					ByteArrayDownload::make(Util::expandQMURL(url.toString(QUrl::FullyEncoded)));
			download->m_followRedirects = true;
			connect(download.get(), &ByteArrayDownload::succeeded, this, &QuickModFilesUpdater::receivedMod);
			connect(download.get(), &ByteArrayDownload::failed, this, &QuickModFilesUpdater::failedMod);
			job->addNetAction(download);
		}
	}
	for (const auto indexUrl : m_list->indices())
	{
		auto download = ByteArrayDownload::make(Util::expandQMURL(indexUrl.toString(QUrl::FullyEncoded)));
		download->m_followRedirects = true;
		connect(download.get(), &ByteArrayDownload::succeeded, this, &QuickModFilesUpdater::receivedMod);
		connect(download.get(), &ByteArrayDownload::failed, this, &QuickModFilesUpdater::failedMod);
		job->addNetAction(download);
	}
	job->start();
}

void QuickModFilesUpdater::receivedMod(int notused)
{
	ByteArrayDownload *download = qobject_cast<ByteArrayDownload *>(sender());
	bool sandbox = download->property("sandbox").toBool();

	// index?
	if (!sandbox)
	{
		try
		{
			const QJsonDocument doc = QJsonDocument::fromJson(download->m_data);
			if (doc.isObject() && doc.object().contains("index"))
			{
				const QJsonObject obj = doc.object();
				QString repo;
				if (obj.contains("repo"))
				{
					repo = MMCJson::ensureString(obj.value("repo"));
					m_list->setRepositoryIndexUrl(repo, download->m_url);
				}
				const QJsonArray array = MMCJson::ensureArray(obj.value("index"));
				for (auto it = array.begin(); it != array.end(); ++it)
				{
					const QJsonObject itemObj = (*it).toObject();
					const QString baseUrlString =
							MMCJson::ensureString(obj.value("baseUrl"));
					const QString uid = MMCJson::ensureString(itemObj.value("uid"));
					if (!m_list->haveUid(QuickModRef(uid), repo))
					{
						const QString urlString =
								MMCJson::ensureString(itemObj.value("url"));
						QUrl url;
						if (baseUrlString.contains("{}"))
						{
							url = QUrl(QString(baseUrlString).replace("{}", urlString));
						}
						else
						{
							url = Util::expandQMURL(baseUrlString)
									.resolved(Util::expandQMURL(urlString));
						}
						registerFile(url, false);
					}
				}
				return;
			}
		}
		catch (MMCError &e)
		{
			QLOG_ERROR() << "Error parsing QuickMod index:" << e.cause();
			return;
		}
	}

	try
	{
		QFile file(
					m_quickmodDir.absoluteFilePath(fileName(MMCJson::ensureString(MMCJson::ensureObject(
																					  MMCJson::parseDocument(download->m_data, QString())).value("uid")))));
		if (file.open(QFile::ReadOnly))
		{
			if (file.readAll() == download->m_data)
			{
				return;
			}
			file.close();
		}
	}
	catch (...)
	{
	}

	auto mod = QuickModPtr(new QuickMod);
	try
	{
		mod->parse(mod, download->m_data);
	}
	catch (MMCError &e)
	{
		QLOG_ERROR() << "QuickMod parse error: " + e.cause();
		QLOG_INFO() << "While reading " << download->m_url.toString();
		emit error(tr("QuickMod parse error"));
		return;
	}

	// assume this is an updated version
	for (auto old : m_list->mods(mod->uid()))
	{
		if (old->repo() == mod->repo())
		{
			m_list->unregisterMod(old);
		}
	}

	auto filename = m_quickmodDir.absoluteFilePath((sandbox ? "sandbox/" : "") + fileName(mod));
	ensureFilePathExists(filename);
	QFile file(filename);
	if (!file.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Failed to open" << file.fileName() << ":" << file.errorString();
		emit error(
					tr("Error opening %1 for writing: %2").arg(file.fileName(), file.errorString()));
		return;
	}
	file.write(download->m_data);
	file.close();

	if (sandbox)
	{
		emit addedSandboxedMod(mod);
	}
	else
	{
		m_list->addMod(mod);
	}
}

void QuickModFilesUpdater::failedMod(int index)
{
	auto download = qobject_cast<ByteArrayDownload *>(sender());
	emit error(tr("Error downloading %1: %2").arg(download->m_url.toString(QUrl::PrettyDecoded),
												  download->m_errorString));
}

void QuickModFilesUpdater::readModFiles()
{
	QLOG_TRACE() << "Reloading quickmod files";
	m_list->clearMods();
	for (const QFileInfo &info :
		 m_quickmodDir.entryInfoList(QStringList() << "*.json", QDir::Files))
	{
		auto mod = QuickModPtr(new QuickMod);
		if (parseQuickMod(info.absoluteFilePath(), mod))
		{
			m_list->addMod(mod);
		}
	}

	update();
}

QString QuickModFilesUpdater::fileName(const QuickModPtr mod)
{
	return fileName(mod->internalUid());
}

QString QuickModFilesUpdater::fileName(const QString &uid)
{
	return uid + ".json";
}

bool QuickModFilesUpdater::parseQuickMod(const QString &fileName, QuickModPtr mod)
{
	QFile file(fileName);
	if (!file.open(QFile::ReadOnly))
	{
		QLOG_ERROR() << "Failed to open" << file.fileName() << ":" << file.errorString();
		emit error(
					tr("Error opening %1 for reading: %2").arg(file.fileName(), file.errorString()));
		return false;
	}

	try
	{
		mod->parse(mod, file.readAll());
	}
	catch (MMCError &e)
	{
		emit error(tr("QuickMod parse error"));
		return false;
	}

	return true;
}
