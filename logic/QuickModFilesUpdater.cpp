#include "QuickModFilesUpdater.h"

#include <QFile>
#include <QTimer>

#include "logic/lists/QuickModsList.h"
#include "logic/net/ByteArrayDownload.h"
#include "logic/net/NetJob.h"

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
		get(m_list->modAt(i)->updateUrl());
	}
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
	emit addedMod(mod);

	QFile file(m_quickmodDir.absoluteFilePath(fileName(mod)));
	if (!file.open(QFile::WriteOnly | QFile::Truncate))
	{
		QLOG_ERROR() << "Failed to open" << file.fileName() << ":" << file.errorString();
		emit error(
			tr("Error opening %1 for writing: %2").arg(file.fileName(), file.errorString()));
		return;
	}
	file.write(download->m_data);
	file.close();
}

void QuickModFilesUpdater::get(const QUrl &url)
{
    auto job = new NetJob("QuickMod download: " + url.toString());
	auto download = ByteArrayDownload::make(url);
    connect(&*download, SIGNAL(succeeded(int)), this, SLOT(receivedMod(int)));
    job->addNetAction(download);
    job->start();
}

void QuickModFilesUpdater::readModFiles()
{
	emit clearMods();
	foreach(const QFileInfo& info,
			m_quickmodDir.entryInfoList(QStringList() << "*_quickmod.json", QDir::Files))
	{
		QFile file(info.filePath());
		if (!file.open(QFile::ReadOnly))
		{
			QLOG_ERROR() << "Failed to open" << file.fileName() << ":" << file.errorString();
			emit error(tr("Error opening %1 for reading: %2")
							.arg(file.fileName(), file.errorString()));
		}

		QuickMod *mod = new QuickMod;
		QString errorMessage;
		mod->parse(file.readAll(), &errorMessage);
		if (!errorMessage.isNull())
		{
			emit error(tr("QuickMod parse error: %1").arg(errorMessage));
			return;
		}

		emit addedMod(mod);
	}
}

QString QuickModFilesUpdater::fileName(const QuickMod *mod)
{
	if (mod->type() == QuickMod::ForgeMod || mod->type() == QuickMod::ForgeCoreMod)
	{
		return QString("other_%1_quickmod.json").arg(mod->name());
	}
	else
	{
		return QString("mod_%1_quickmod.json").arg(mod->modId());
	}
}
