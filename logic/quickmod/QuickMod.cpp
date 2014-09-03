/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "QuickMod.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

#include "logic/net/CacheDownload.h"
#include "logic/net/NetJob.h"
#include "MultiMC.h"
#include "QuickModVersion.h"
#include "QuickModsList.h"
#include "modutils.h"
#include "logic/MMCJson.h"

#define CURRENT_QUICKMOD_VERSION 1

QuickModRef::QuickModRef() : m_uid(QString())
{
}

QuickModRef::QuickModRef(const QString &uid) : m_uid(uid)
{
}

QuickModRef::QuickModRef(const QString &uid, const QUrl &updateUrl)
	: m_uid(uid), m_updateUrl(updateUrl)
{
}

QString QuickModRef::userFacing() const
{
	const auto mod = findMod();
	return mod ? mod->name() : m_uid;
}

QString QuickModRef::toString() const
{
	return m_uid;
}

QuickModPtr QuickModRef::findMod() const
{
	const auto mods = findMods();
	if (mods.isEmpty())
	{
		return QuickModPtr();
	}
	return mods.first();
}

QList<QuickModPtr> QuickModRef::findMods() const
{
	return MMC->quickmodslist()->mods(*this);
}

QList<QuickModVersionRef> QuickModRef::findVersions() const
{
	QList<QuickModVersionRef> versions;
	for (const auto mod : findMods())
	{
		for (const auto version : mod->versions())
		{
			if (!versions.contains(version))
			{
				versions.append(version);
			}
		}
	}
	return versions;
}

QDebug operator<<(QDebug dbg, const QuickModRef &uid)
{
	return dbg << QString("QuickModRef(%1)").arg(uid.toString()).toLatin1().constData();
}

uint qHash(const QuickModRef &uid)
{
	return qHash(uid.toString());
}

QuickMod::QuickMod(QObject *parent) : QObject(parent), m_imagesLoaded(false)
{
}

QuickMod::~QuickMod()
{
	m_versions.clear(); // as they are shared pointers this will also delete them
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

QList<QuickModVersionRef> QuickMod::versions() const
{
	QList<QuickModVersionRef> refs;
	for (const auto version : m_versions)
	{
		refs.append(version);
	}
	return refs;
}

QuickModVersionRef QuickMod::latestVersion(const QString &mcVersion) const
{
	for (auto version : m_versions)
	{
		if (version->compatibleVersions.contains(mcVersion))
		{
			return version;
		}
	}
	return QuickModVersionRef();
}

void QuickMod::sortVersions()
{
	std::sort(m_versions.begin(), m_versions.end(),
			  [](const QuickModVersionPtr v1, const QuickModVersionPtr v2)
	{ return v1->m_version > v2->m_version; });
}

void QuickMod::parse(QuickModPtr _this, const QByteArray &data)
{
	const QJsonDocument doc = MMCJson::parseDocument(data, "QuickMod file");
	const QJsonObject mod = MMCJson::ensureObject(doc, "root");

	auto version = MMCJson::ensureInteger(mod.value("formatVersion"), "'formatVersion'");
	if (version > CURRENT_QUICKMOD_VERSION)
	{
		throw MMCError(tr("QuickMod format to new"));
	}

	m_uid = QuickModRef(MMCJson::ensureString(mod.value("uid"), "'uid'"));
	m_repo = MMCJson::ensureString(mod.value("repo"), "'repo'");
	m_name = MMCJson::ensureString(mod.value("name"), "'name'");
	m_description = mod.value("description").toString();
	m_modId = mod.value("modId").toString();
	m_license = mod.value("license").toString();

	if (mod.contains("authors"))
	{
		auto authors = MMCJson::ensureObject(mod.value("authors"), "'authors'");
		for (auto it = authors.begin(); it != authors.end(); ++it)
		{
			m_authors[it.key()] =
				MMCJson::ensureStringList(it.value(), "authors array");
		}
	}

	if (mod.contains("urls"))
	{
		auto urls = MMCJson::ensureObject(mod.value("urls"), "'urls'");
		for (auto it = urls.begin(); it != urls.end(); ++it)
		{
			const QJsonArray list = MMCJson::ensureArray(it.value());
			QList<QUrl> urlList;
			for (auto listItem : list)
			{
				urlList.append(QUrl(MMCJson::ensureString(listItem, "url item")));
			}
			m_urls[it.key()] = urlList;
		}
	}

	m_references.clear();
	const QJsonObject references = mod.value("references").toObject();
	for (auto key : references.keys())
	{
		m_references.insert(QuickModRef(key), QUrl(MMCJson::ensureString(
												  references[key], "'reference'")));
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

	m_updateUrl = QUrl(MMCJson::ensureString(mod.value("updateUrl"), "'updateUrl'"));

	m_versions.clear();
	for (auto val : MMCJson::ensureArray(mod.value("versions"), "'versions'"))
	{
		QuickModVersionPtr ptr(new QuickModVersion(_this));
		ptr->parse(MMCJson::ensureObject(val, "version"));
		m_versions.append(ptr);
	}
	sortVersions();

	if (!m_uid.isValid())
	{
		throw QuickModParseError("The 'uid' field in the QuickMod file for " +
								 m_name + " is empty");
	}
	if (m_repo.isEmpty())
	{
		throw QuickModParseError("The 'repo' field in the QuickMod file for " +
								 m_name + " is empty");
	}
}

QByteArray QuickMod::toJson() const
{
	using namespace MMCJson;
	QJsonObject obj;
	obj.insert("formatVersion", 1);
	obj.insert("uid", m_uid.toString());
	obj.insert("repo", m_repo);
	obj.insert("name", m_name);
	obj.insert("updateUrl", m_updateUrl.toString(QUrl::FullyEncoded));
	writeString(obj, "modId", m_modId);
	writeString(obj, "description", m_description);
	writeString(obj, "license", m_license);
	writeStringList(obj, "tags", m_tags);
	writeStringList(obj, "categories", m_categories);
	if (!m_authors.isEmpty())
	{
		QJsonObject authors;
		for (auto it = m_authors.constBegin(); it != m_authors.constEnd(); ++it)
		{
			authors.insert(it.key(), QJsonArray::fromStringList(it.value()));
		}
		obj.insert("authors", authors);
	}
	if (!m_urls.isEmpty())
	{
		QJsonObject urls;
		for (auto it = m_urls.constBegin(); it != m_urls.constEnd(); ++it)
		{
			QJsonArray array;
			for (const auto url : it.value())
			{
				array.append(url.toString(QUrl::FullyEncoded));
			}
			urls.insert(it.key(), array);
		}
		obj.insert("urls", urls);
	}
	writeObjectList(obj, "versions", m_versions);
	return QJsonDocument(obj).toJson();
}

bool QuickMod::compare(const QuickModPtr other) const
{
	return m_name == other->m_name || m_uid == other->m_uid;
}

QuickMod::UrlType QuickMod::urlType(const QString &id)
{
	if (id == "website")
	{
		return Website;
	}
	else if (id == "wiki")
	{
		return Wiki;
	}
	else if (id == "forum")
	{
		return Forum;
	}
	else if (id == "donation")
	{
		return Donation;
	}
	else if (id == "issues")
	{
		return Issues;
	}
	else if (id == "source")
	{
		return Source;
	}
	else if (id == "icon")
	{
		return Icon;
	}
	else if (id == "logo")
	{
		return Logo;
	}
	return Invalid;
}

QString QuickMod::urlId(const QuickMod::UrlType type)
{
	switch (type)
	{
	case Website:
		return "website";
	case Wiki:
		return "wiki";
	case Forum:
		return "forum";
	case Donation:
		return "donation";
	case Issues:
		return "issues";
	case Source:
		return "source";
	case Icon:
		return "icon";
	case Logo:
		return "logo";
	default:
		return QString();
	}
}

QString QuickMod::humanUrlId(const QuickMod::UrlType type)
{
	switch (type)
	{
	case Website:
		return tr("Website");
	case Wiki:
		return tr("Wiki");
	case Forum:
		return tr("Forum");
	case Donation:
		return tr("Donation");
	case Issues:
		return tr("Issues");
	case Source:
		return tr("Source");
	case Icon:
		return tr("Icon");
	case Logo:
		return tr("Logo");
	default:
		return QString();
	}
}

QList<QuickMod::UrlType> QuickMod::urlTypes()
{
	return QList<UrlType>() << Website << Wiki << Forum << Donation << Issues << Source << Icon
							<< Logo;
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
	if (iconUrl().isValid() && m_icon.isNull())
	{
		auto icon = CacheDownload::make(
			iconUrl(), MMC->metacache()->resolveEntry("quickmod/icons", fileName(iconUrl())));
		connect(icon.get(), &CacheDownload::succeeded, this, &QuickMod::iconDownloadFinished);
		job->addNetAction(icon);
		download = true;
	}
	if (logoUrl().isValid() && m_logo.isNull())
	{
		auto logo = CacheDownload::make(
			logoUrl(), MMC->metacache()->resolveEntry("quickmod/logos", fileName(logoUrl())));
		connect(logo.get(), &CacheDownload::succeeded, this, &QuickMod::logoDownloadFinished);
		job->addNetAction(logo);
		download = true;
	}
	if (download)
	{
		job->start();
		//FIXME: job is leaked here.
	}
	else
	{
		delete job;
	}
}

QString QuickMod::fileName(const QUrl &url) const
{
	const QString path = url.path();
	return internalUid() + path.mid(path.lastIndexOf("."));
}

QStringList QuickMod::mcVersions()
{
	if (!m_mcVersionListCache.isEmpty())
	{
		return m_mcVersionListCache;
	}
	
	for (auto quickModV : versionsInternal())
	{
		for (const QString mcv : quickModV->compatibleVersions)
		{
			if (!m_mcVersionListCache.contains(mcv))
			{
				m_mcVersionListCache.append(mcv);
			}
		}
	}
	return m_mcVersionListCache;
}
