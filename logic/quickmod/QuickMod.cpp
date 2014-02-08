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

#define MALFORMED_JSON_X(message)                                                              \
	if (errorMessage)                                                                          \
	{                                                                                          \
		*errorMessage = message;                                                               \
	}                                                                                          \
	return false
#define MALFORMED_JSON MALFORMED_JSON_X(tr("Malformed QuickMod file"))
#define JSON_ASSERT_X(assertation, message)                                                    \
	if (!(assertation))                                                                        \
	{                                                                                          \
		MALFORMED_JSON_X(message);                                                             \
	}                                                                                          \
	qt_noop()
#define JSON_ASSERT(assertation)                                                               \
	if (!(assertation))                                                                        \
	{                                                                                          \
		MALFORMED_JSON;                                                                        \
	}                                                                                          \
	qt_noop()
bool QuickMod::parse(const QByteArray &data, QString *errorMessage)
{
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(data, &error);
	JSON_ASSERT_X(error.error == QJsonParseError::NoError, error.errorString() + " at " + QString::number(error.offset));
	JSON_ASSERT(doc.isObject());
	QJsonObject mod = doc.object();
	m_stub = mod.value("stub").toBool(false);
	m_uid = mod.value("uid").toString();
	m_name = mod.value("name").toString();
	m_description = mod.value("description").toString();
	m_nemName = mod.value("nemName").toString();
	m_modId = mod.value("modId").toString();
	m_websiteUrl = Util::expandQMURL(mod.value("websiteUrl").toString());
	m_verifyUrl = Util::expandQMURL(mod.value("verifyUrl").toString());
	m_iconUrl = Util::expandQMURL(mod.value("iconUrl").toString());
	m_logoUrl = Util::expandQMURL(mod.value("logoUrl").toString());
	m_updateUrl = Util::expandQMURL(mod.value("updateUrl").toString());
	m_references.clear();
	QVariantMap references = mod.value("references").toObject().toVariantMap();
	for (auto key : references.keys())
	{
		m_references.insert(key, Util::expandQMURL(references[key].toString()));
	}
	m_categories.clear();
	foreach(const QJsonValue & val, mod.value("categories").toArray())
	{
		m_categories.append(val.toString());
	}
	m_tags.clear();
	foreach(const QJsonValue & val, mod.value("tags").toArray())
	{
		m_tags.append(val.toString());
	}
	m_versionsUrl = Util::expandQMURL(mod.value("versionsUrl").toString());

	if (uid().isNull() || uid().isEmpty())
	{
		QLOG_INFO() << "QuickMod" << m_name << ":";
		MALFORMED_JSON_X(
			tr("There needs to be a 'uid' field in the QuickMod file"));
	}

	return true;
}
#undef MALFORMED_JSON_X
#undef MALFORMED_JSON
#undef JSON_ASSERT_X
#undef JSON_ASSERT

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
