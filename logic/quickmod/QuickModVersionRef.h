#pragma once
#include "QuickMod.h"
#include <modutils.h>

class QuickModVersionRef
{
	QuickModRef m_mod;
	QString m_id;
	Util::Version m_version;

public:
	explicit QuickModVersionRef(const QuickModRef &mod, const QString &id, const Util::Version &version);
	explicit QuickModVersionRef(const QuickModRef &mod, const QString &id);
	QuickModVersionRef()
	{
	}
	QuickModVersionRef(const QuickModVersionPtr &ptr);

	bool isValid() const
	{
		return m_mod.isValid() && !m_id.isEmpty();
	}
	QString userFacing() const;
	QuickModPtr findMod() const;
	QuickModVersionPtr findVersion() const;

	bool operator<(const QuickModVersionRef &other) const;
	bool operator<=(const QuickModVersionRef &other) const;
	bool operator>(const QuickModVersionRef &other) const;
	bool operator>=(const QuickModVersionRef &other) const;
	bool operator==(const QuickModVersionRef &other) const;

	QString toString() const
	{
		return m_id;
	}
	QuickModRef mod() const
	{
		return m_mod;
	}
};
Q_DECLARE_METATYPE(QuickModVersionRef)
