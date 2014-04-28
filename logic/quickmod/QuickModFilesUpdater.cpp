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

#include "logger/QsLog.h"

QuickModFilesUpdater::QuickModFilesUpdater(QuickModsList *list) : QObject(list), m_list(list)
{
	m_quickmodDir = QDir::current();
	m_quickmodDir.mkdir("quickmod");
	m_quickmodDir.cd("quickmod");

	QTimer::singleShot(1, this, SLOT(readModFiles()));
}

void QuickModFilesUpdater::registerFile(const QUrl &url)
{
	auto job = new NetJob("QuickMod download");
	auto download = ByteArrayDownload::make(Util::expandQMURL(url.toString(QUrl::FullyEncoded)));
	download->m_followRedirects = true;
	connect(download.get(), SIGNAL(succeeded(int)), this, SLOT(receivedMod(int)));
	connect(download.get(), SIGNAL(failed(int)), this, SLOT(failedMod(int)));
	job->addNetAction(download);
	job->start();
}

void QuickModFilesUpdater::unregisterMod(const QuickModPtr mod)
{
	m_quickmodDir.remove(fileName(mod));
}

void QuickModFilesUpdater::update()
{
	auto job = new NetJob("QuickMod download");
	for (int i = 0; i < m_list->numMods(); ++i)
	{
		auto url = m_list->modAt(i)->updateUrl();
		if (url.isValid())
		{
			auto download = ByteArrayDownload::make(Util::expandQMURL(url.toString(QUrl::FullyEncoded)));
			download->m_followRedirects = true;
			connect(download.get(), SIGNAL(succeeded(int)), this, SLOT(receivedMod(int)));
			connect(download.get(), SIGNAL(failed(int)), this, SLOT(failedMod(int)));
			job->addNetAction(download);
		}
	}
	connect(job, &NetJob::succeeded, [this]()
	{
		NetJob *versionsJob = new NetJob("QuickMod Versions Update");
		for (int i = 0; i < m_list->numMods(); ++i)
		{
			auto mod = m_list->modAt(i);
			auto entry =
				MMC->metacache()->resolveEntry("quickmod/versions", mod->uid() + ".json");
			entry->stale = true;

			CacheDownloadPtr download;
			versionsJob->addNetAction(download =
										  CacheDownload::make(mod->versionsUrl(), entry));
			download->m_followRedirects = true;
			connect(download.get(), &CacheDownload::succeeded, [mod, download](int)
			{
				try
				{
					const QJsonDocument doc = MMCJson::parseFile(download->getTargetFilepath(),
																 "QuickMod Version file");
					QJsonArray root = doc.array();
					QList<QuickModVersionPtr> versions;
					for (auto value : root)
					{
						QuickModVersionPtr version =
							QuickModVersionPtr(new QuickModVersion(mod, true));
						version->parse(value.toObject());
						versions.append(version);
					}
					mod->setVersions(versions);
				}
				catch (MMCError &e)
				{
					QLOG_ERROR() << e.cause();
				}
			});
		}
		versionsJob->start();
	});
	job->start();
}

void QuickModFilesUpdater::receivedMod(int notused)
{
	ByteArrayDownload *download = qobject_cast<ByteArrayDownload *>(sender());

	// index?
	try
	{
		const QJsonDocument doc = QJsonDocument::fromJson(download->m_data);
		if (doc.isObject() && doc.object().contains("index"))
		{
			const QJsonObject obj = doc.object();
			const QJsonArray array = MMCJson::ensureArray(obj.value("index"));
			for (auto it = array.begin(); it != array.end(); ++it)
			{
				const QJsonObject itemObj = (*it).toObject();
				const QString baseUrlString =
					MMCJson::ensureUrl(obj.value("baseUrl")).toString();
				if (!m_list->haveUid(MMCJson::ensureString(itemObj.value("uid"))))
				{
					const QString urlString =
						MMCJson::ensureUrl(itemObj.value("url")).toString();
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
					registerFile(url);
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

	try
	{
		QFile file(m_quickmodDir.absoluteFilePath(fileName(MMCJson::ensureString(
			MMCJson::ensureObject(MMCJson::parseDocument(download->m_data, QString())).value("uid")))));
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
		mod->parse(download->m_data);
	}
	catch (MMCError &e)
	{
		QLOG_ERROR() << "QuickMod parse error: " + e.cause();
		QLOG_INFO() << "While reading " << download->m_url.toString();
		emit error(tr("QuickMod parse error"));
		return;
	}

	mod->m_hash = QCryptographicHash::hash(download->m_data, QCryptographicHash::Sha512);

	// assume this is an updated version
	if (QuickModPtr old = m_list->mod(mod->uid()))
	{
		m_list->unregisterMod(old);
	}

	QFile file(m_quickmodDir.absoluteFilePath(fileName(mod)));
	if (!file.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Failed to open" << file.fileName() << ":" << file.errorString();
		emit error(
			tr("Error opening %1 for writing: %2").arg(file.fileName(), file.errorString()));
		return;
	}
	file.write(download->m_data);
	file.close();

	m_list->addMod(mod);
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
	foreach(const QFileInfo & info,
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
	return fileName(mod->uid());
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
		mod->parse(file.readAll());
	}
	catch (MMCError &e)
	{
		emit error(tr("QuickMod parse error"));
		return false;
	}

	return true;
}
