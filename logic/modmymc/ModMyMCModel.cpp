#include "ModMyMCModel.h"

#include <QIcon>
#include <QTextDocumentFragment>

#include "logic/net/CacheDownload.h"
#include "logic/net/NetJob.h"
#include "logic/net/HttpMetaCache.h"
#include "logic/MMCJson.h"
#include "MultiMC.h"
#include "BuildConfig.h"

ModMyMCModel::ModMyMCModel(QObject *parent) : QAbstractListModel(parent)
{
}

int ModMyMCModel::rowCount(const QModelIndex &parent) const
{
	return m_entries.size();
}
QVariant ModMyMCModel::data(const QModelIndex &index, int role) const
{
	Entry entry = m_entries[index.row()];
	if (role == Qt::DisplayRole)
	{
		return entry.mod;
	}
	else if (role == Qt::DecorationRole)
	{
		return entry.type == Entry::NewMod ? QIcon::fromTheme("new") : QIcon::fromTheme("copy");
	}
	else if (role == Qt::ToolTipRole)
	{
		return entry.changelog;
	}
	return QVariant();
}

void ModMyMCModel::update()
{
	if (m_currentDownload)
	{
		return;
	}

	NetJob *job = new NetJob("ModMyMC Download");
	job->addNetAction(
		m_currentDownload = CacheDownload::make(
			BuildConfig.MODMYMC_URL, MMC->metacache()->resolveEntry("root", "modmymc.json")));
	connect(job, &NetJob::succeeded, [this, job]()
			{
		using namespace MMCJson;

		QList<Entry> entries;
		for (const auto val :
			 ensureArray(parseFile(m_currentDownload->getTargetFilepath(), "ModMyMC data")))
		{
			const QJsonObject obj = ensureObject(val);
			entries.append(createEntry(
				ensureString(obj.value("data")),
				QDateTime::fromString(ensureString(obj.value("timestamp")), Qt::ISODate)));
		}
		std::sort(entries.begin(), entries.end(), [](Entry e1, Entry e2)
				  {
			return e1.timestamp > e2.timestamp;
		});

		beginResetModel();
		m_entries = entries;
		endResetModel();

		m_currentDownload.reset();
		job->deleteLater();
	});
	connect(job, &NetJob::failed, [this, job]()
			{
		m_currentDownload.reset();
		job->deleteLater();
	});
	job->start();
}

ModMyMCModel::Entry ModMyMCModel::createEntry(const QString &data, const QDateTime &timestamp)
{
	Entry entry;
	QString cooked = QString(data).remove("http://t.co/uuhlkgwiKm").trimmed();
	if (cooked.startsWith("Mod Update: "))
	{
		entry.type = Entry::UpdatedMod;
		cooked = cooked.remove("Mod Update: ");
	}
	else if (cooked.startsWith("New Mod: "))
	{
		entry.type = Entry::NewMod;
		cooked = cooked.remove("New Mod: ");
	}
	const QString decoded = QTextDocumentFragment::fromHtml(cooked).toPlainText();
	const int separatorIndex = decoded.indexOf(" - ");
	entry.mod = separatorIndex == -1 ? decoded : decoded.left(separatorIndex).trimmed();
	entry.changelog =
		separatorIndex == -1 ? QString() : decoded.mid(separatorIndex + 3).trimmed();
	entry.timestamp = timestamp;
	return entry;
}
