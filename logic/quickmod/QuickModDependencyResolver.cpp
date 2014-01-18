#include "QuickModDependencyResolver.h"

#include <QWidget>

#include "gui/dialogs/VersionSelectDialog.h"
#include "QuickModVersion.h"
#include "QuickMod.h"
#include "QuickModsList.h"
#include "MultiMC.h"
#include "modutils.h"

int qHash(const QuickModVersionPtr &ptr, const uint seed = 0)
{
	return qHash(ptr->mod->uid() + ptr->name(), seed);
}

QuickModDependencyResolver::QuickModDependencyResolver(BaseInstance *instance, QWidget *parent)
	: QuickModDependencyResolver(instance, parent, parent)
{

}

QuickModDependencyResolver::QuickModDependencyResolver(BaseInstance *instance, QWidget *widgetParent, QObject *parent)
	: QObject(parent), m_widgetParent(widgetParent), m_instance(instance)
{

}

QSet<QuickModVersionPtr> QuickModDependencyResolver::resolve(const QList<QuickMod *> &mods)
{
	foreach (QuickMod *mod, mods)
	{
		m_mods.insert(mod, qMakePair(mod->versions().toSet(), Explicit));
		addDependencies(mod);
	}
	reduce();
	useLatest();
	QSet<QuickModVersionPtr> out;
	typedef QPair<QSet<QuickModVersionPtr>, QuickModDependencyResolver::Type> VersionTypePair;
	foreach (const VersionTypePair &pair, m_mods.values())
	{
		out += pair.first;
	}
	return out;
}

void QuickModDependencyResolver::addDependencies(QuickMod *mod)
{
	foreach (const QString &other, mod->references().keys())
	{
		addDependencies(MMC->quickmodslist()->mod(other));
	}
	m_mods.insert(mod, qMakePair(mod->versions().toSet(), Implicit));
}

bool QuickModDependencyResolver::isUsed(const QuickMod *mod) const
{
	for (auto modIt = m_mods.begin(); modIt != m_mods.end(); ++modIt)
	{
		auto versions = modIt.value().first;
		for (auto verIt = versions.begin(); verIt != versions.end(); ++verIt)
		{
			QuickModVersionPtr version = *verIt;
			if (version->dependencies.contains(mod->uid()) || version->breaks.contains(mod->uid())
					|| version->provides.contains(mod->uid()))
			{
				return true;
			}
		}
	}
	return false;
}
bool QuickModDependencyResolver::isLatest(const QuickModVersionPtr &ptr, const QSet<QuickModVersionPtr> &set) const
{
	for (auto it = set.begin(); it != set.end(); ++it)
	{
		if (Util::Version(ptr->name()) < Util::Version((*it)->name()))
		{
			return false;
		}
	}
	return true;
}

void QuickModDependencyResolver::reduce()
{
	typedef QPair<QSet<QuickModVersionPtr>, QuickModDependencyResolver::Type> VersionTypePair;
	// remove no longer used mods
	{
		QMutableHashIterator<QuickMod *, VersionTypePair> it(m_mods);
		while (it.hasNext())
		{
			if (!isUsed(it.next().key()))
			{
				it.remove();
			}
		}
	}

	// apply version filters
	{
		for (auto it = m_mods.begin(); it != m_mods.end(); ++it)
		{
			QMutableSetIterator<QuickModVersionPtr> verIt(it.value().first);
			while (verIt.hasNext())
			{
				QuickModVersionPtr version = verIt.next();
				for (auto depIt = version->dependencies.begin(); depIt != version->dependencies.end(); ++depIt)
				{
					applyVersionFilter(MMC->quickmodslist()->mod(depIt.key()), depIt.value());
				}
			}
		}
	}

	// apply breaks
	{
		// TODO apply breaks
	}

	// remove mods without versions
	{
		QMutableHashIterator<QuickMod *, VersionTypePair> it(m_mods);
		while (it.hasNext())
		{
			// TODO if it's an explicit mod we're in trouble
			if (it.next().value().first.isEmpty())
			{
				it.remove();
			}
		}
	}
}
void QuickModDependencyResolver::applyVersionFilter(QuickMod *mod, const QString &filter)
{
	auto it = m_mods.find(mod);
	QMutableSetIterator<QuickModVersionPtr> verIt(it.value().first);
	while (verIt.hasNext())
	{
		if (!Util::versionIsInInterval(verIt.next()->name(), filter))
		{
			verIt.remove();
		}
	}
}
void QuickModDependencyResolver::useLatest()
{
	for (auto it = m_mods.begin(); it != m_mods.end(); ++it)
	{
		QMutableSetIterator<QuickModVersionPtr> verIt(it.value().first);
		while (verIt.hasNext())
		{
			if (!isLatest(verIt.next(), it.value().first))
			{
				verIt.remove();
			}
		}
	}
}

QuickModVersionPtr QuickModDependencyResolver::getVersion(QuickMod *mod, const QString &filter, bool *ok)
{
	VersionSelectDialog dialog(new QuickModVersionList(mod, m_instance, this),
							   tr("Choose QuickMod version for %1").arg(mod->name()), m_widgetParent);
	dialog.setFilter(BaseVersionList::NameColumn, filter);
	if (dialog.exec() == QDialog::Rejected)
	{
		*ok = false;
		return QuickModVersionPtr();
	}
	*ok = true;
	return std::dynamic_pointer_cast<QuickModVersion>(dialog.selectedVersion());
}
