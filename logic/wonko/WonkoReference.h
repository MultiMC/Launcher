#pragma once

#include <QString>
#include <QMetaType>

#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT WonkoReference
{
public:
	WonkoReference() {}
	explicit WonkoReference(const QString &uid);

	QString uid() const;

	QString version() const;
	void setVersion(const QString &version);

	bool operator==(const WonkoReference &other) const;
	bool operator!=(const WonkoReference &other) const;

private:
	QString m_uid;
	QString m_version;
};
Q_DECLARE_METATYPE(WonkoReference)
