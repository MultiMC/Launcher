/* Copyright 2014 MultiMC Contributors
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

#pragma once

#include "logic/quickmod/QuickMod.h"
#include "logic/quickmod/QuickModVersion.h"

class QuickModVersionBuilder;

class QuickModBuilder
{
public:
	QuickModBuilder();

	QuickModBuilder setUid(const QString &uid)
	{
		m_mod->m_uid = QuickModRef(uid);
		return *this;
	}

	QuickModBuilder setName(const QString &name)
	{
		m_mod->m_name = name;
		return *this;
	}

	QuickModBuilder setRepo(const QString &repo)
	{
		m_mod->m_repo = repo;
		return *this;
	}

	QuickModBuilder setDescription(const QString &description)
	{
		m_mod->m_description = description;
		return *this;
	}

	QuickModBuilder setUpdateUrl(const QUrl &url)
	{
		m_mod->m_updateUrl = url;
		return *this;
	}

	QuickModBuilder setNemName(const QString &nemName)
	{
		m_mod->m_nemName = nemName;
		return *this;
	}

	QuickModBuilder setModId(const QString &modid)
	{
		m_mod->m_modId = modid;
		return *this;
	}

	QuickModBuilder setLicense(const QString &license)
	{
		m_mod->m_license = license;
		return *this;
	}

	QuickModBuilder setTags(const QStringList &tags)
	{
		m_mod->m_tags = tags;
		return *this;
	}

	QuickModBuilder setCategories(const QStringList &categories)
	{
		m_mod->m_categories = categories;
		return *this;
	}

	QuickModBuilder setMavenUrls(const QList<QUrl> &urls)
	{
		m_mod->m_mavenRepos = urls;
		return *this;
	}

	QuickModBuilder addUrl(const QString &type, const QUrl &url)
	{
		m_mod->m_urls[type].append(url);
		return *this;
	}

	QuickModBuilder addUrl(const QuickMod::UrlType type, const QUrl &url)
	{
		m_mod->m_urls[QuickMod::urlId(type)].append(url);
		return *this;
	}

	QuickModBuilder addAuthor(const QString &type, const QString &name)
	{
		m_mod->m_authors[type].append(name);
		return *this;
	}

	QuickModVersionBuilder addVersion();
	QuickModBuilder addVersion(const QuickModVersionPtr ptr);

	QuickModPtr build()
	{
		m_mod->sortVersions();
		return m_mod;
	}

private:
	friend class QuickModVersionBuilder;
	QuickModPtr m_mod;

	void finishVersion(QuickModVersionPtr version);
};

class QuickModVersionBuilder
{
public:
	QuickModVersionBuilder setName(const QString &name)
	{
		m_version->name_ = name;
		return *this;
	}
	QuickModVersionBuilder setVersion(const QString &version)
	{
		m_version->version_string = version;
		m_version->m_version = Util::Version(version);
		return *this;
	}
	QuickModVersionBuilder setType(const QString &type)
	{
		m_version->type = type;
		return *this;
	}
	QuickModVersionBuilder setCompatibleMCVersions(const QStringList &versions)
	{
		m_version->compatibleVersions = versions;
		return *this;
	}
	QuickModVersionBuilder setForgeVersionFilter(const QString &forgeVersionFilter)
	{
		m_version->forgeVersionFilter = forgeVersionFilter;
		return *this;
	}
	QuickModVersionBuilder setInstallType(const QuickModVersion::InstallType &type)
	{
		m_version->installType = type;
		return *this;
	}
	QuickModVersionBuilder setSha1(const QString &sha1)
	{
		m_version->sha1 = sha1;
		return *this;
	}

	QuickModVersionBuilder addDependency(const QuickModRef &uid, const QuickModVersionRef &version, const bool isSoft = false)
	{
		m_version->dependencies.insert(uid, qMakePair(version, isSoft));
		return *this;
	}
	QuickModVersionBuilder addRecommendation(const QuickModRef &uid, const QuickModVersionRef &version)
	{
		m_version->recommendations.insert(uid, version);
		return *this;
	}
	QuickModVersionBuilder addSuggestion(const QuickModRef &uid, const QuickModVersionRef &version)
	{
		m_version->suggestions.insert(uid, version);
		return *this;
	}
	QuickModVersionBuilder addConflicts(const QuickModRef &uid, const QuickModVersionRef &version)
	{
		m_version->conflicts.insert(uid, version);
		return *this;
	}
	QuickModVersionBuilder addProvides(const QuickModRef &uid, const QuickModVersionRef &version)
	{
		m_version->provides.insert(uid, version);
		return *this;
	}

	QuickModVersionBuilder addLibrary(const QString &name, const QUrl &url)
	{
		m_version->libraries.append(QuickModVersion::Library(name, url));
		return *this;
	}
	QuickModVersionBuilder addDownload(const QUrl &url,
									   const QuickModDownload::DownloadType type,
									   const int priority, const QString &hint,
									   const QString &group)
	{
		QuickModDownload download;
		download.url = url.toString(QUrl::FullyEncoded);
		download.type = type;
		download.priority = priority;
		download.hint = hint;
		download.group = group;
		m_version->downloads.append(download);
		return *this;
	}

	QuickModBuilder build();

private:
	friend class QuickModBuilder;
	QuickModVersionBuilder(QuickModBuilder builder);
	QuickModVersionPtr m_version;
	QuickModBuilder m_builder;
};

