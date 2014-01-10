#include "QuickMod.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

#include "logic/net/CacheDownload.h"
#include "logic/net/NetJob.h"
#include "MultiMC.h"
#include "QuickModVersion.h"

QuickMod::QuickMod(QObject *parent)
	: QObject(parent), m_stub(false), m_imagesLoaded(false)
{
}

QString QuickMod::uid() const
{
	if (!m_uid.isEmpty())
	{
		return m_uid;
	}
	else if (!m_modId.isEmpty())
	{
		return m_modId;
	}
	else
	{
		return QString();
	}
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
	JSON_ASSERT_X(error.error == QJsonParseError::NoError, error.errorString());
	JSON_ASSERT(doc.isObject());
	QJsonObject mod = doc.object();
	m_stub = mod.value("stub").toBool(false);
	m_uid = mod.value("uid").toString();
	m_name = mod.value("name").toString();
	m_description = mod.value("description").toString();
	m_nemName = mod.value("nemName").toString();
	m_modId = mod.value("modId").toString();
	m_websiteUrl = QUrl(mod.value("websiteUrl").toString());
	m_verifyUrl = QUrl(mod.value("verifyUrl").toString());
	m_iconUrl = QUrl(mod.value("iconUrl").toString());
	m_logoUrl = QUrl(mod.value("logoUrl").toString());
	m_updateUrl = QUrl(mod.value("updateUrl").toString());
	m_references.clear();
	QVariantMap references = mod.value("references").toObject().toVariantMap();
	for (auto key : references.keys())
	{
		m_references.insert(key, QUrl(references[key].toString()));
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
	const QString modType = mod.value("type").toString();
	if (modType == "forgeMod")
	{
		m_type = ForgeMod;
	}
	else if (modType == "forgeCoreMod")
	{
		m_type = ForgeCoreMod;
	}
	else if (modType == "resourcePack")
	{
		m_type = ResourcePack;
	}
	else if (modType == "configPack")
	{
		m_type = ConfigPack;
	}
	else if (modType == "group")
	{
		m_type = Group;
	}
	else
	{
		MALFORMED_JSON_X(tr("Unknown version type: %1").arg(modType));
	}
	m_versionsUrl = QUrl(mod.value("versionsUrl").toString());

	if (uid().isNull())
	{
		MALFORMED_JSON_X(
			tr("There needs to be either a 'uid' or a 'modId' field in the QuickMod file"));
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
	m_icon = QIcon(download->m_target_path);
	if (!m_icon.isNull())
	{
		emit iconUpdated();
	}
}
void QuickMod::logoDownloadFinished(int index)
{
	auto download = qobject_cast<CacheDownload *>(sender());
	m_logo = QPixmap(download->m_target_path);
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
