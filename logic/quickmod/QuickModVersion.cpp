#include "QuickModVersion.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "logic/net/HttpMetaCache.h"
#include "QuickMod.h"
#include "MultiMC.h"

static QMap<QString, QString> jsonObjectToStringStringMap(const QJsonObject &obj)
{
	QMap<QString, QString> out;
	for (auto it = obj.begin(); it != obj.end(); ++it)
	{
		out.insert(it.key(), it.value().toString());
	}
	return out;
}

bool QuickModVersion::parse(const QJsonObject &object, QString *errorMessage)
{
	name_ = object.value("name").toString();
	url = QUrl(object.value("url").toString());
	// checksum
	{
		if (object.contains("checksum_md4"))
		{
			checksum = object.value("checksum_md4").toString().toLatin1();
			checksum_algorithm = QCryptographicHash::Md4;
		}
		else if (object.contains("checksum_md5"))
		{
			checksum = object.value("checksum_md5").toString().toLatin1();
			checksum_algorithm = QCryptographicHash::Md5;
		}
		else if (object.contains("checksum_sha1"))
		{
			checksum = object.value("checksum_sha1").toString().toLatin1();
			checksum_algorithm = QCryptographicHash::Sha1;
		}
		else if (object.contains("checksum_sha224"))
		{
			checksum = object.value("checksum_sha224").toString().toLatin1();
			checksum_algorithm = QCryptographicHash::Sha224;
		}
		else if (object.contains("checksum_sha256"))
		{
			checksum = object.value("checksum_sha256").toString().toLatin1();
			checksum_algorithm = QCryptographicHash::Sha256;
		}
		else if (object.contains("checksum_sha384"))
		{
			checksum = object.value("checksum_sha384").toString().toLatin1();
			checksum_algorithm = QCryptographicHash::Sha384;
		}
		else if (object.contains("checksum_sha512"))
		{
			checksum = object.value("checksum_sha512").toString().toLatin1();
			checksum_algorithm = QCryptographicHash::Sha512;
		}
		else if (object.contains("checksum_sha3_224"))
		{
			checksum = object.value("checksum_sha3_224").toString().toLatin1();
			checksum_algorithm = QCryptographicHash::Sha3_224;
		}
		else if (object.contains("checksum_sha3_256"))
		{
			checksum = object.value("checksum_sha3_256").toString().toLatin1();
			checksum_algorithm = QCryptographicHash::Sha3_256;
		}
		else if (object.contains("checksum_sha3_384"))
		{
			checksum = object.value("checksum_sha3_384").toString().toLatin1();
			checksum_algorithm = QCryptographicHash::Sha3_384;
		}
		else if (object.contains("checksum_sha3_512"))
		{
			checksum = object.value("checksum_sha3_512").toString().toLatin1();
			checksum_algorithm = QCryptographicHash::Sha3_512;
		}
		else
		{
			checksum = QByteArray();
		}
	}
	forgeVersionFilter = object.value("forgeCompat").toString();
	compatibleVersions.clear();
	foreach(const QJsonValue & val, object.value("mcCompat").toArray())
	{
		compatibleVersions.append(val.toString());
	}
	dependencies = jsonObjectToStringStringMap(object.value("depends").toObject());
	recommendations = jsonObjectToStringStringMap(object.value("recommends").toObject());
	suggestions = jsonObjectToStringStringMap(object.value("suggests").toObject());
	breaks = jsonObjectToStringStringMap(object.value("breaks").toObject());
	conflicts = jsonObjectToStringStringMap(object.value("conflicts").toObject());
	provides = jsonObjectToStringStringMap(object.value("provides").toObject());

	// download type
	{
		const QString typeString = object.value("downloadType").toString("parallel");
		if (typeString == "direct")
		{
			downloadType = Direct;
		}
		else if (typeString == "parallel")
		{
			downloadType = Parallel;
		}
		else if (typeString == "sequential")
		{
			downloadType = Sequential;
		}
		else
		{
			*errorMessage = QObject::tr("Unknown value for \"downloadType\" field");
			return false;
		}
	}
	// install type
	{
		const QString typeString = object.value("installType").toString("forgeMod");
		if (typeString == "forgeMod")
		{
			installType = ForgeMod;
		}
		else if (typeString == "forgeCoreMod")
		{
			installType = ForgeCoreMod;
		}
		else if (typeString == "extract")
		{
			installType = Extract;
		}
		else if (typeString == "configPack")
		{
			installType = ConfigPack;
		}
		else if (typeString == "group")
		{
			installType = Group;
		}
		else
		{
			*errorMessage = QObject::tr("Unknown value for \"installType\" field");
			return false;
		}
	}
	return true;
}

QuickModVersionPtr QuickModVersion::invalid(QuickMod *mod)
{
	return QuickModVersionPtr(new QuickModVersion(mod, false));
}

QuickModVersionList::QuickModVersionList(QuickMod *mod, BaseInstance *instance, QObject *parent)
	: BaseVersionList(parent), m_mod(mod), m_instance(instance)
{
}

Task *QuickModVersionList::getLoadTask()
{
	return new QuickModVersionListLoadTask(this);
}

bool QuickModVersionList::isLoaded()
{
	return m_loaded;
}

const BaseVersionPtr QuickModVersionList::at(int i) const
{
	return m_vlist.at(i);
}

int QuickModVersionList::count() const
{
	return m_vlist.count();
}

void QuickModVersionList::sort()
{
	qSort(m_vlist.begin(), m_vlist.end(), [](const BaseVersionPtr v1, const BaseVersionPtr v2)
	{
		return std::dynamic_pointer_cast<QuickModVersion>(v1)->name() >
			   std::dynamic_pointer_cast<QuickModVersion>(v2)->name();
	});
}

void QuickModVersionList::updateListData(QList<BaseVersionPtr> versions)
{
	beginResetModel();
	m_vlist = versions;
	m_loaded = true;
	endResetModel();
	sort();
}

QuickModVersionListLoadTask::QuickModVersionListLoadTask(QuickModVersionList *vlist)
	: Task(), m_vlist(vlist)
{
}

void QuickModVersionListLoadTask::executeTask()
{
	setStatus(tr("Fetching QuickMod version list..."));
	auto job = new NetJob("Version list");
	auto entry =
		MMC->metacache()->resolveEntry("quickmod/versions", m_vlist->m_mod->uid() + ".json");
	entry->stale = true;

	job->addNetAction(listDownload = CacheDownload::make(m_vlist->m_mod->versionsUrl(), entry));

	connect(listDownload.get(), SIGNAL(failed(int)), SLOT(listFailed()));

	listJob.reset(job);
	connect(listJob.get(), SIGNAL(succeeded()), SLOT(listDownloaded()));
	connect(listJob.get(), SIGNAL(progress(qint64, qint64)), this,
			SIGNAL(progress(qint64, qint64)));
	listJob->start();
}

void QuickModVersionListLoadTask::listDownloaded()
{
	setStatus(tr("Parsing reply..."));
	QList<QuickModVersionPtr> list;
	QJsonParseError error;
	QFile file(listDownload->m_target_path);
	if (!file.open(QFile::ReadOnly))
	{
		QLOG_ERROR() << "Couldn't open QuickMod version file " << file.fileName()
					 << " for parsing: " << file.errorString();
		emitFailed(tr("Couldn't parse reply. See the log for details."));
		return;
	}
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		QLOG_ERROR() << "Error parsing JSON in " << file.fileName() << ":"
					 << error.errorString();
		emitFailed(tr("Couldn't parse reply. See the log for details."));
		return;
	}
	QJsonArray root = doc.array();
	foreach(const QJsonValue & value, root)
	{
		QuickModVersionPtr version = QuickModVersionPtr(new QuickModVersion(m_vlist->m_mod, true));
		QString errorMessage;
		version->parse(value.toObject(), &errorMessage);
		if (!errorMessage.isNull())
		{
			QLOG_ERROR() << "Error parsing JSON in " << file.fileName() << ":" << errorMessage;
			emitFailed(tr("Couldn't parse reply. See the log for details."));
			return;
		}
		list.append(version);
	}

	QList<BaseVersionPtr> baseList;
	foreach (QuickModVersionPtr ptr, list)
	{
		baseList.append(ptr);
	}

	m_vlist->m_mod->setVersions(list);
	m_vlist->updateListData(baseList);
	emitSucceeded();
	return;
}

void QuickModVersionListLoadTask::listFailed()
{
	auto reply = listDownload->m_reply;
	if (reply)
	{
		QLOG_ERROR() << "Getting QuickMod version list failed for " << m_vlist->m_mod->name()
					 << ": " << reply->errorString();
	}
	else
	{
		QLOG_ERROR() << "Getting QuickMod version list failed for reasons unknown.";
	}
}
