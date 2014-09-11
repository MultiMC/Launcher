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

#include "QuickModBuilder.h"

#include "logic/quickmod/QuickModsList.h"
#include "MultiMC.h"

QuickModVersionBuilder::QuickModVersionBuilder(QuickModBuilder builder)
	: m_version(std::make_shared<QuickModVersion>(builder.m_mod)), m_builder(builder)
{
}
QuickModBuilder QuickModVersionBuilder::build()
{
	m_builder.finishVersion(m_version);
	return m_builder;
}

QuickModBuilder::QuickModBuilder() : m_mod(std::make_shared<QuickModMetadata>())
{
}

QuickModVersionBuilder QuickModBuilder::addVersion()
{
	return QuickModVersionBuilder(*this);
}

QuickModBuilder QuickModBuilder::addVersion(const QuickModVersionPtr ptr)
{
	auto builder = addVersion()
					   .setName(ptr->name())
					   .setType(ptr->type)
					   .setInstallType(ptr->installType)
					   .setSha1(ptr->sha1)
					   .setForgeVersionFilter(ptr->forgeVersionFilter)
					   .setCompatibleMCVersions(ptr->compatibleVersions);
	builder.m_version->dependencies = ptr->dependencies;
	builder.m_version->recommendations = ptr->recommendations;
	builder.m_version->suggestions = ptr->suggestions;
	builder.m_version->conflicts = ptr->conflicts;
	builder.m_version->provides = ptr->provides;
	return builder.build();
}

QJsonObject QuickModBuilder::build() const
{
	QJsonObject obj = m_mod->toJson();
	QJsonArray versions;
	for (const auto version : m_versions)
	{
		versions.append(version->toJson());
	}
	obj.insert("versions", versions);
	return obj;
}

void QuickModBuilder::finishVersion(QuickModVersionPtr version)
{
	Q_ASSERT(version->mod == m_mod);
	m_versions.append(version);
	const auto deps =
		QList<QuickModRef>() << version->dependencies.keys() << version->recommendations.keys()
							 << version->suggestions.keys() << version->conflicts.keys()
							 << version->provides.keys();
	for (const auto dep : deps)
	{
		if (m_mod->m_references.contains(dep))
		{
			continue;
		}
		const QList<QuickModMetadataPtr> ptrs = MMC->quickmodslist()->mods(dep);
		if (ptrs.isEmpty())
		{
			continue;
		}
		m_mod->m_references.insert(dep, ptrs.first()->updateUrl());
	}
}
