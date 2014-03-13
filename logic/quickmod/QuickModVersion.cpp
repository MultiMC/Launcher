#include "QuickModVersion.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "logic/net/HttpMetaCache.h"
#include "QuickMod.h"
#include "MultiMC.h"
#include "logic/MMCJson.h"

static QMap<QString, QString> jsonObjectToStringStringMap(const QJsonObject &obj)
{
	QMap<QString, QString> out;
	for (auto it = obj.begin(); it != obj.end(); ++it)
	{
		out.insert(it.key(), MMCJson::ensureString(it.value()));
	}
	return out;
}

bool QuickModVersion::parse(const QJsonObject &object, QString *errorMessage)
{
	name_ = MMCJson::ensureString(object.value("name"), "'name'");
	url = MMCJson::ensureUrl(object.value("url"), "'url'");
	md5 = object.value("md5").toString();
	forgeVersionFilter = object.value("forgeCompat").toString();
	compatibleVersions.clear();
	for (auto val : MMCJson::ensureArray(object.value("mcCompat"), "'mcCompat'"))
	{
		compatibleVersions.append(MMCJson::ensureString(val));
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
	QFile file(listDownload->getTargetFilepath());
	if (!file.open(QFile::ReadOnly))
	{
		QLOG_ERROR() << "Couldn't open QuickMod version file " << file.fileName()
					 << " for parsing: " << file.errorString();
		emitFailed(tr("Couldn't parse reply. See the log for details."));
		return;
	}
	try
	{
		const QJsonDocument doc = MMCJson::parseDocument(file.readAll(), "QuickMod Version file");
		QJsonArray root = doc.array();
		for (auto value : root)
		{
			QuickModVersionPtr version = QuickModVersionPtr(new QuickModVersion(m_vlist->m_mod, true));
			version->parse(value.toObject());
			list.append(version);
		}
	}
	catch (MMCError &e)
	{
		QLOG_ERROR() << "Error parsing JSON in " << file.fileName() << ":"
					 << e.cause();
		emitFailed(tr("Couldn't parse reply. See the log for details."));
		return;
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
