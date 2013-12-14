#include "QuickModFilesUpdater.h"

#include <QFile>
#include <QTimer>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonObject>

#include "logic/lists/QuickModsList.h"
#include "logic/net/ByteArrayDownload.h"
#include "logic/net/NetJob.h"
#include "logic/Mod.h"

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
	get(url);
}

void QuickModFilesUpdater::update()
{
	for (int i = 0; i < m_list->numMods(); ++i)
	{
		auto url = m_list->modAt(i)->updateUrl();
		if (url.isValid())
		{
			get(url);
		}
	}
}

QuickMod *QuickModFilesUpdater::ensureExists(const Mod &mod)
{
	auto qMod = new QuickMod;
	qMod->m_name = mod.name();
	qMod->m_modId = mod.id();
	qMod->m_websiteUrl = QUrl(mod.homeurl());
	qMod->m_description = mod.description();
	qMod->m_stub = true;
	QuickMod::Version version;
	version.name = mod.version();
	qMod->m_versions.append(version);
	if (mod.filename().path().contains("coremod"))
	{
		qMod->m_type = QuickMod::ForgeCoreMod;
	}
	else
	{
		qMod->m_type = QuickMod::ForgeMod;
	}

	QFileInfo modFile(m_quickmodDir.absoluteFilePath(fileName(qMod)));
	// we save a new file if: there isn't an old one OR it's not parseable OR it's a stub to
	if (!modFile.exists())
	{
		saveQuickMod(qMod);
	}
	else
	{
		auto oldMod = new QuickMod;
		if (!parseQuickMod(modFile.absoluteFilePath(), oldMod))
		{
			saveQuickMod(qMod);
		}
		else if (oldMod->isStub())
		{
			saveQuickMod(qMod);
		}
		else
		{
			delete qMod;
			qMod = oldMod;
		}
	}

	return qMod;
}

void QuickModFilesUpdater::receivedMod(int notused)
{
	ByteArrayDownload *download = qobject_cast<ByteArrayDownload*>(sender());
	QuickMod *mod = new QuickMod;
	QString errorMessage;
	mod->parse(download->m_data, &errorMessage);
	if (!errorMessage.isNull())
	{
		emit error(tr("QuickMod parse error: %1").arg(errorMessage));
		return;
	}

	m_quickmodDir.remove(fileName(mod));

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
	emit error(tr("Error downloading %1: %2").arg(download->m_url.toString(QUrl::PrettyDecoded), download->m_errorString));
}

void QuickModFilesUpdater::get(const QUrl &url)
{
	auto job = new NetJob("QuickMod download: " + url.toString());
	auto download = ByteArrayDownload::make(url);
	connect(&*download, SIGNAL(succeeded(int)), this, SLOT(receivedMod(int)));
	connect(download.get(), SIGNAL(failed(int)), this, SLOT(failedMod(int)));
	job->addNetAction(download);
	job->start();
}

void QuickModFilesUpdater::readModFiles()
{
	m_list->clearMods();
	foreach(const QFileInfo& info,
			  m_quickmodDir.entryInfoList(QStringList() << "*_quickmod.json", QDir::Files))
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
	switch (mod->type())
	{
	case QuickMod::ForgeMod:
		obj.insert("type", QStringLiteral("forgeMod"));
		break;
	case QuickMod::ForgeCoreMod:
		obj.insert("type", QStringLiteral("forgeCoreMod"));
		break;
	case QuickMod::ResourcePack:
		obj.insert("type", QStringLiteral("resourcepack"));
		break;
	case QuickMod::ConfigPack:
		obj.insert("type", QStringLiteral("configpack"));
		break;
	}

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
	if (mod->type() != QuickMod::ForgeMod && mod->type() != QuickMod::ForgeCoreMod)
	{
		return QString("other_%1_quickmod.json").arg(mod->name());
	}
	else
	{
		return QString("mod_%1_quickmod.json").arg(mod->modId().isNull() ? mod->name() : mod->modId());
	}
}

bool QuickModFilesUpdater::parseQuickMod(const QString &fileName, QuickMod *mod)
{
	QLOG_INFO() << "Saving the QuickMod file" << fileName;
	QFile file(fileName);
	if (!file.open(QFile::ReadOnly))
	{
		QLOG_ERROR() << "Failed to open" << file.fileName() << ":" << file.errorString();
		emit error(tr("Error opening %1 for reading: %2")
				   .arg(file.fileName(), file.errorString()));
		return false;
	}

	QString errorMessage;
	mod->parse(file.readAll(), &errorMessage);
	if (!errorMessage.isNull())
	{
		emit error(tr("QuickMod parse error: %1").arg(errorMessage));
		return false;
	}

	return true;
}
