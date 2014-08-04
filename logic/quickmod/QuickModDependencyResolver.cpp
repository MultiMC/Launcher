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

#include "QuickModDependencyResolver.h"

#include <QDebug>

#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickMod.h"
#include "logic/quickmod/QuickModsList.h"
#include "logic/OneSixInstance.h"
#include "MultiMC.h"
#include "modutils.h"

struct DepNode
{
	QuickModVersionRef version;
	QuickModRef uid;
	QList<const DepNode *> children;
	QList<const DepNode *> parents;
	bool isHard;

	QList<const DepNode *> getParents() const
	{
		QList<const DepNode *> out = parents;
		for (const auto child : parents)
		{
			out.append(child->getParents());
		}
		return out;
	}
	bool hasHardParent() const
	{
		if (isHard)
		{
			return true;
		}
		for (const auto parent : parents)
		{
			if (parent->hasHardParent())
			{
				return true;
			}
		}
		return false;
	}

	static QList<const DepNode *> build(std::shared_ptr<OneSixInstance> instance, bool *ok = 0)
	{
		if (ok)
		{
			*ok = true;
		}

		QMap<QuickModRef, const DepNode *> nodes;

		// stage one: create nodes
		{
			const auto mods = instance->getFullVersion()->quickmods;
			for (auto it = mods.constBegin(); it != mods.constEnd(); ++it)
			{
				Q_ASSERT(!nodes.contains(it.key()));
				DepNode *node = new DepNode;
				node->uid = it.key();
				node->isHard = it.value().second;
				if (it.value().first.isValid())
				{
					node->version = it.value().first;
				}
				else
				{
					if (ok)
					{
						*ok = false;
					}
				}
				nodes.insert(it.key(), node);
			}
		}

		// stage two: forward dependencies (children)
		{
			for (auto it = nodes.constBegin(); it != nodes.constEnd(); ++it)
			{
				const DepNode *node = it.value();
				if (!node->version.findVersion())
				{
					if (ok)
					{
						*ok = false;
					}
					continue;
				}
				const auto ptr = node->version.findVersion();
				for (auto versionIt = ptr->dependencies.constBegin();
					 versionIt != ptr->dependencies.constEnd(); ++versionIt)
				{
					if (!versionIt.value().second && nodes.contains(versionIt.key()))
					{
						const_cast<DepNode *>(node)
							->children.append(nodes.value(versionIt.key()));
					}
					else
					{
						if (ok)
						{
							*ok = false;
						}
					}
				}
			}
		}

		// stage three: backward dependencies (parents)
		{
			for (const auto node : nodes)
			{
				for (const auto dep : node->children)
				{
					const_cast<DepNode *>(dep)->parents.append(node);
				}
			}
		}

		return nodes.values();
	}

	static const DepNode *findNode(const QList<const DepNode *> &nodes, const QuickModRef &uid)
	{
		for (const auto node : nodes)
		{
			if (node->uid == uid)
			{
				return node;
			}
		}
		return nullptr;
	}
};

QuickModDependencyResolver::QuickModDependencyResolver(std::shared_ptr<OneSixInstance> instance,
													   Bindable *parent)
	: QObject(0), Bindable(parent), m_instance(instance)
{
}

QList<QuickModVersionPtr> QuickModDependencyResolver::resolve(const QList<QuickModRef> &mods)
{
	for (QuickModRef mod : mods)
	{
		bool ok;
		resolve(getVersion(mod, QuickModVersionRef(), &ok));
		if (!ok)
		{
			emit error(tr("Didn't select a version for %1").arg(mod.userFacing()));
			return QList<QuickModVersionPtr>();
		}
	}
	return m_mods.values();
}

QList<QuickModRef> QuickModDependencyResolver::resolveChildren(const QList<QuickModRef> &uids)
{
	QList<const DepNode *> nodes = DepNode::build(m_instance);
	QList<const DepNode *> parents;
	for (const auto uid : uids)
	{
		const DepNode *node = DepNode::findNode(nodes, uid);
		parents.append(node);
		parents.append(node->getParents());
	}
	QList<QuickModRef> out;
	for (const auto node : parents)
	{
		if (!out.contains(node->uid))
		{
			out.append(node->uid);
		}
	}
	return out;
}

QList<QuickModRef> QuickModDependencyResolver::resolveOrphans() const
{
	QList<QuickModRef> orphans;
	QList<const DepNode *> nodes = DepNode::build(m_instance);
	for (const auto uid : m_instance->getFullVersion()->quickmods.keys())
	{
		const auto node = DepNode::findNode(nodes, uid);
		Q_ASSERT(node);
		if (!node->hasHardParent())
		{
			orphans.append(uid);
		}
	}
	return orphans;
}

bool QuickModDependencyResolver::hasResolveError() const
{
	bool ok = false;
	DepNode::build(m_instance, &ok);
	return !ok;
}

QuickModVersionPtr QuickModDependencyResolver::getVersion(const QuickModRef &modUid,
														  const QuickModVersionRef &filter, bool *ok)
{
	return wait<QuickModVersionPtr>("QuickMods.GetVersion", modUid, filter, ok);
}

void QuickModDependencyResolver::resolve(const QuickModVersionPtr version)
{
	if (!version)
	{
		return;
	}
	if (m_mods.contains(version->mod.get()) &&
		Util::Version(version->name()) <= Util::Version(m_mods[version->mod.get()]->name()))
	{
		return;
	}
	m_mods.insert(version->mod.get(), version);
	for (auto it = version->dependencies.begin(); it != version->dependencies.end(); ++it)
	{
		QuickModVersionPtr dep;

		if (!MMC->quickmodslist()->mods(it.key()).isEmpty())
		{
			bool ok;
			dep = getVersion(it.key(), it.value().first, &ok);
			if (!ok)
			{
				emit error(tr("Didn't select a version while resolving from %1 (%2) to %3").arg(
					version->mod->name(), version->name(), it.key().toString()));
			}
		}
		else
		{
			QList<QuickModVersionRef> versions =
				MMC->quickmodslist()->modsProvidingModVersion(it.key(), it.value().first);
			if (!versions.isEmpty())
			{
				for (QuickModVersionRef providingVersion : versions)
				{
					if (m_mods.values().contains(providingVersion.findVersion()))
					{
						// found already added mod
						dep = providingVersion.findVersion();
						break;
					}
				}
				if (!dep)
				{
					// no dependency added...
					// TODO show a dialog to select mod and version
					dep = versions.first().findVersion();
				}
			}
		}

		if (!dep)
		{
			emit warning(tr("The dependency from %1 (%2) to %3 cannot be resolved")
							 .arg(version->mod->name(), version->name(), it.key().toString()));
			continue;
		}

		if (dep)
		{
			emit success(tr("Successfully resolved dependency from %1 (%2) to %3 (%4)").arg(
				version->mod->name(), version->name(), dep->mod->name(), dep->name()));
			if (m_mods.contains(version->mod.get()) &&
				Util::Version(dep->name()) > Util::Version(m_mods[version->mod.get()]->name()))
			{
				resolve(dep);
			}
			else
			{
				resolve(dep);
			}
		}
		else
		{
			emit error(tr("The dependency from %1 (%2) to %3 (%4) cannot be resolved").arg(
				version->mod->name(), version->name(), it.key().toString(), it.value().first.toString()));
		}
	}
}
