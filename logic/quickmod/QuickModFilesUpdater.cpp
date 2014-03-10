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
#include "modutils.h"

#include "logger/QsLog.h"

QuickModFilesUpdater::QuickModFilesUpdater(QuickModsList *list) : QObject(list), m_list(list)
{
	m_quickmodDir = QDir::current();
	m_quickmodDir.mkdir("quickmod");
	m_quickmodDir.cd("quickmod");

	QTimer::singleShot(1, this, SLOT(readModFiles()));
	QTimer::singleShot(2, this, SLOT(update()));
}

void QuickModFilesUpdater::registerFile(const QUrl &url)
{
	auto job = new NetJob("QuickMod download");
	auto download =
		ByteArrayDownload::make(Util::expandQMURL(url.toString(QUrl::FullyEncoded)));
	connect(download.get(), SIGNAL(succeeded(int)), this, SLOT(receivedMod(int)));
	connect(download.get(), SIGNAL(failed(int)), this, SLOT(failedMod(int)));
	job->addNetAction(download);
	job->start();
}

void QuickModFilesUpdater::unregisterMod(const QuickMod *mod)
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
			auto download =
				ByteArrayDownload::make(Util::expandQMURL(url.toString(QUrl::FullyEncoded)));
			connect(download.get(), SIGNAL(succeeded(int)), this, SLOT(receivedMod(int)));
			connect(download.get(), SIGNAL(failed(int)), this, SLOT(failedMod(int)));
			job->addNetAction(download);
		}
	}
	job->start();
}

QuickMod *QuickModFilesUpdater::ensureExists(const Mod &mod)
{
	if (QuickMod *qMod = m_list->modForModId(mod.mod_id().isEmpty() ? mod.name() : mod.mod_id()))
	{
		if (!qMod->isStub())
		{
			return qMod;
		}
	}

	auto qMod = new QuickMod;
	qMod->m_name = mod.name();
	qMod->m_modId = mod.mod_id().isEmpty() ? mod.name() : mod.mod_id();
	qMod->m_uid = qMod->modId();
	qMod->m_websiteUrl = QUrl(mod.homeurl());
	qMod->m_description = mod.description();
	qMod->m_stub = true;

	saveQuickMod(qMod);

	return qMod;
}

void QuickModFilesUpdater::receivedMod(int notused)
{
	ByteArrayDownload *download = qobject_cast<ByteArrayDownload *>(sender());

	// index?
	{
		const QJsonObject obj = QJsonDocument::fromJson(download->m_data).object();
		if (obj.contains("index") && obj.value("index").isArray())
		{
			const QJsonArray array = obj.value("index").toArray();
			for (auto it = array.begin(); it != array.end(); ++it)
			{
				const QJsonObject itemObj = (*it).toObject();
				const QString baseUrlString = obj.value("baseUrl").toString();
				if (!m_list->haveUid(itemObj.value("uid").toString()))
				{
					const QString urlString = itemObj.value("url").toString();
					QUrl url;
					if (baseUrlString.contains("{}"))
					{
						url = QUrl(QString(baseUrlString).replace("{}", urlString));
					}
					else
					{
						url = Util::expandQMURL(baseUrlString).resolved(Util::expandQMURL(itemObj.value("url").toString()));
					}
					registerFile(url);
				}
			}
			return;
		}
	}

	auto mod = new QuickMod;
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
	if (QuickMod *old = m_list->mod(mod->uid()))
	{
		m_list->unregisterMod(old);
	}
	if (!mod->modId().isEmpty())
	{
		while (QuickMod *old = m_list->modForModId(mod->modId()))
		{
			m_list->unregisterMod(old);
		}
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
		auto mod = new QuickMod;
		if (parseQuickMod(info.absoluteFilePath(), mod))
		{
			m_list->addMod(mod);
		}
	}
}

void QuickModFilesUpdater::saveQuickMod(QuickMod *mod)
{
	QJsonObject obj;
	obj.insert("name", mod->name());
	obj.insert("modId", mod->modId());
	obj.insert("websiteUrl", mod->websiteUrl().toString());
	obj.insert("description", mod->description());
	obj.insert("stub", mod->isStub());

	QFile file(m_quickmodDir.absoluteFilePath(fileName(mod)));
	if (!file.open(QFile::WriteOnly | QFile::Truncate))
	{
		QLOG_ERROR() << "Failed to open" << file.fileName() << ":" << file.errorString();
		emit error(
			tr("Error opening %1 for writing: %2").arg(file.fileName(), file.errorString()));
		return;
	}
	file.write(QJsonDocument(obj).toJson());
	file.close();

	m_list->addMod(mod);
}

QString QuickModFilesUpdater::fileName(const QuickMod *mod)
{
	return mod->uid() + ".json";
}

bool QuickModFilesUpdater::parseQuickMod(const QString &fileName, QuickMod *mod)
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
