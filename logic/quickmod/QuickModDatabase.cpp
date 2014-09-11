#include "QuickModDatabase.h"

#include <QDir>
#include <QCoreApplication>
#include <QTimer>

#include <pathutils.h>

#include "logic/quickmod/QuickModsList.h"
#include "logic/MMCJson.h"

QString QuickModDatabase::m_filename = QDir::current().absoluteFilePath("quickmods/quickmods.json");

QuickModDatabase::QuickModDatabase(QuickModsList *list)
	: QObject(list), m_list(list), m_timer(new QTimer(this))
{
	ensureFolderPathExists(QDir::current().absoluteFilePath("quickmods/"));

	connect(qApp, &QCoreApplication::aboutToQuit, this, &QuickModDatabase::flushToDisk);
	connect(m_timer, &QTimer::timeout, this, &QuickModDatabase::flushToDisk);

	syncFromDisk();
}

void QuickModDatabase::add(QuickModMetadataPtr metadata)
{
	m_metadata[metadata->uid().toString()][metadata->repo()] = metadata;

	delayedFlushToDisk();
}
void QuickModDatabase::add(QuickModVersionPtr version)
{
	// TODO merge versions
	m_versions[version->mod->uid().toString()][version->version().toString()] = version;

	delayedFlushToDisk();
}

QHash<QuickModRef, QList<QuickModMetadataPtr>> QuickModDatabase::metadata() const
{
	QHash<QuickModRef, QList<QuickModMetadataPtr>> out;
	for (auto uidIt = m_metadata.constBegin(); uidIt != m_metadata.constEnd(); ++uidIt)
	{
		out.insert(QuickModRef(uidIt.key()), uidIt.value().values());
	}
	return out;
}

void QuickModDatabase::delayedFlushToDisk()
{
	m_isDirty = true;
	m_timer->start(1000); // one second
}
void QuickModDatabase::flushToDisk()
{
	if (!m_isDirty)
	{
		return;
	}
	QJsonObject root;

	for (auto it = m_metadata.constBegin(); it != m_metadata.constEnd(); ++it)
	{
		QJsonObject metadata;
		for (auto metaIt = it.value().constBegin(); metaIt != it.value().constEnd(); ++metaIt)
		{
			metadata[metaIt.key()] = metaIt.value()->toJson();
		}

		auto versionsHash = m_versions[it.key()];
		QJsonObject versions;
		for (auto versionIt = versionsHash.constBegin(); versionIt != versionsHash.constEnd(); ++versionIt)
		{
			QJsonObject vObj = versionIt.value()->toJson();
			versions[versionIt.key()] = vObj;
		}

		QJsonObject obj;
		obj.insert("metadata", metadata);
		obj.insert("versions", versions);
		root.insert(it.key(), obj);
	}

	QFile file(m_filename);
	if (!file.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Unable to save QuickMod Database to disk:" << file.errorString();
		return;
	}
	file.write(QJsonDocument(root).toBinaryData());

	m_isDirty = false;
}

void QuickModDatabase::syncFromDisk()
{
	using namespace MMCJson;
	try
	{
		emit aboutToReset();
		m_metadata.clear();
		m_versions.clear();
		const QJsonObject root = ensureObject(MMCJson::parseFile(m_filename, "QuickMod Database"));
		for (auto it = root.constBegin(); it != root.constEnd(); ++it)
		{
			const QString uid = it.key();
			const QJsonObject obj = ensureObject(it.value());

			// metadata
			{
				const QJsonObject metadata = ensureObject(obj.value("metadata"));
				for (auto metaIt = metadata.constBegin(); metaIt != metadata.constEnd(); ++metaIt)
				{
					QuickModMetadataPtr ptr = std::make_shared<QuickModMetadata>();
					ptr->parse(MMCJson::ensureObject(metaIt.value()));
					m_metadata[uid][metaIt.key()] = ptr;
				}
			}

			// versions
			{
				const QJsonObject versions = ensureObject(obj.value("versions"));
				for (auto versionIt = versions.constBegin(); versionIt != versions.constEnd(); ++versionIt)
				{
					QuickModVersionPtr ptr = std::make_shared<QuickModVersion>();
					ptr->parse(MMCJson::ensureObject(versionIt.value()));
					m_versions[uid][versionIt.key()] = ptr;
				}
			}
		}
	}
	catch (MMCError &e)
	{
		QLOG_ERROR() << "Error while reading QuickMod Database:" << e.cause();
	}
	emit reset();
}
