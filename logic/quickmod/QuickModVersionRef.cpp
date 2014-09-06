#include "QuickModVersion.h"
#include "QuickModVersionRef.h"
#include <QString>

QuickModVersionRef::QuickModVersionRef(const QuickModRef &mod, const QString &id,
									   const Util::Version &version)
	: m_mod(mod), m_id(id), m_version(version)
{
}
QuickModVersionRef::QuickModVersionRef(const QuickModRef &mod, const QString &id)
	: m_mod(mod), m_id(id), m_version(id)
{
}
QuickModVersionRef::QuickModVersionRef(const QuickModVersionPtr &ptr)
	: QuickModVersionRef(ptr->version())
{
}

QString QuickModVersionRef::userFacing() const
{
	const QuickModVersionPtr ptr = findVersion();
	return ptr ? ptr->name() : QString();
}
QuickModPtr QuickModVersionRef::findMod() const
{
	const QuickModVersionPtr ptr = findVersion();
	return ptr ? ptr->mod : QuickModPtr();
}
QuickModVersionPtr QuickModVersionRef::findVersion() const
{
	if (!isValid())
	{
		return QuickModVersionPtr();
	}
	QList<QuickModPtr> mods = m_mod.findMods();
	for (const auto mod : mods)
	{
		for (const auto version : mod->versionsInternal())
		{
			if (version->version() == *this)
			{
				return version;
			}
		}
	}

	for (const auto mod : mods)
	{
		for (const auto version : mod->versionsInternal())
		{
			if (version->version() == *this)
			{
				return version;
			}
		}
	}

	return QuickModVersionPtr();
}

bool QuickModVersionRef::operator<(const QuickModVersionRef &other) const
{
	Q_ASSERT(m_mod == other.m_mod);
	return m_version < other.m_version;
}
bool QuickModVersionRef::operator<=(const QuickModVersionRef &other) const
{
	Q_ASSERT(m_mod == other.m_mod);
	return m_version <= other.m_version;
}
bool QuickModVersionRef::operator>(const QuickModVersionRef &other) const
{
	Q_ASSERT(m_mod == other.m_mod);
	return m_version > other.m_version;
}
bool QuickModVersionRef::operator>=(const QuickModVersionRef &other) const
{
	Q_ASSERT(m_mod == other.m_mod);
	return m_version >= other.m_version;
}
bool QuickModVersionRef::operator==(const QuickModVersionRef &other) const
{
	return m_mod == other.m_mod && m_version == other.m_version;
}
