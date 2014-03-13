#include "QuickMod.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

#include "logic/net/CacheDownload.h"
#include "logic/net/NetJob.h"
#include "MultiMC.h"
#include "QuickModVersion.h"
#include "modutils.h"
#include "logic/MMCJson.h"

QuickMod::QuickMod(QObject *parent)
	: QObject(parent), m_stub(false), m_imagesLoaded(false)
{
}

QIcon QuickMod::icon()
{
	fetchImages();
	return m_icon;
}
QPixmap QuickMod::logo()
{
	fetchImages();
	return m_logo;
}

QuickModVersionPtr QuickMod::version(const QString &name) const
{
	foreach (QuickModVersionPtr ptr, m_versions)
	{
		if (ptr->name() == name)
		{
			return ptr;
		}
	}
	return QuickModVersionPtr();
}

void QuickMod::parse(const QByteArray &data)
{
	const QJsonDocument doc = MMCJson::parseDocument(data, "QuickMod file");
	const QJsonObject mod = MMCJson::ensureObject(doc, "root");
	m_uid = MMCJson::ensureString(mod.value("uid"), "'uid'");
	m_name = MMCJson::ensureString(mod.value("name"), "'name'");
	m_stub = mod.value("stub").toBool(false);
	m_description = mod.value("description").toString();
	m_nemName = mod.value("nemName").toString();
	m_modId = mod.value("modId").toString();
	m_websiteUrl = Util::expandQMURL(mod.value("websiteUrl").toString());
	m_verifyUrl = Util::expandQMURL(mod.value("verifyUrl").toString());
	m_iconUrl = Util::expandQMURL(mod.value("iconUrl").toString());
	m_logoUrl = Util::expandQMURL(mod.value("logoUrl").toString());
	m_references.clear();
	const QJsonObject references = mod.value("references").toObject();
	for (auto key : references.keys())
	{
		m_references.insert(key, Util::expandQMURL(MMCJson::ensureString(references[key], "'reference'")));
	}
	m_categories.clear();
	for (auto val : mod.value("categories").toArray())
	{
		m_categories.append(MMCJson::ensureString(val, "'category'"));
	}
	m_tags.clear();
	for (auto val : mod.value("tags").toArray())
	{
		m_tags.append(MMCJson::ensureString(val, "'tag'"));
	}
	m_versionsUrl = Util::expandQMURL(MMCJson::ensureString(mod.value("versionsUrl"), "'versionsUrl'"));
	m_updateUrl = Util::expandQMURL(MMCJson::ensureString(mod.value("updateUrl"), "'updateUrl'"));

	if (uid().isEmpty())
	{
		throw QuickModParseError("The 'uid' field in the QuickMod file for " + m_name + " is empty");
	}
}

bool QuickMod::compare(const QuickMod *other) const
{
	return m_name == other->name();
}

void QuickMod::iconDownloadFinished(int index)
{
	auto download = qobject_cast<CacheDownload *>(sender());
	m_icon = QIcon(download->getTargetFilepath());
	if (!m_icon.isNull())
	{
		emit iconUpdated();
	}
}
void QuickMod::logoDownloadFinished(int index)
{
	auto download = qobject_cast<CacheDownload *>(sender());
	m_logo = QPixmap(download->getTargetFilepath());
	if (!m_logo.isNull())
	{
		emit logoUpdated();
	}
}

void QuickMod::fetchImages()
{
	if (m_imagesLoaded)
	{
		return;
	}
	auto job = new NetJob("QuickMod image download: " + m_name);
	bool download = false;
	m_imagesLoaded = true;
	if (m_iconUrl.isValid() && m_icon.isNull())
	{
		auto icon = CacheDownload::make(
			m_iconUrl, MMC->metacache()->resolveEntry("quickmod/icons", fileName(m_iconUrl)));
		connect(icon.get(), &CacheDownload::succeeded, this, &QuickMod::iconDownloadFinished);
		job->addNetAction(icon);
		download = true;
	}
	if (m_logoUrl.isValid() && m_logo.isNull())
	{
		auto logo = CacheDownload::make(
			m_logoUrl, MMC->metacache()->resolveEntry("quickmod/logos", fileName(m_logoUrl)));
		connect(logo.get(), &CacheDownload::succeeded, this, &QuickMod::logoDownloadFinished);
		job->addNetAction(logo);
		download = true;
	}
	if (download)
	{
		job->start();
	}
	else
	{
		delete job;
	}
}

QString QuickMod::fileName(const QUrl &url) const
{
	const QString path = url.path();
	return uid() + path.mid(path.lastIndexOf("."));
}
