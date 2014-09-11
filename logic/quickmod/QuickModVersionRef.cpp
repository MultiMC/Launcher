#include "QuickModVersion.h"

#include <QString>

#include "QuickModVersionRef.h"
#include "QuickModsList.h"
#include "MultiMC.h"

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
	const QuickModVersionPtr ptr = MMC->quickmodslist()->version(*this);
	return ptr ? ptr->name() : QString();
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

uint qHash(const QuickModVersionRef &ref)
{
	return qHash(ref.mod().toString() + ref.toString());
}
