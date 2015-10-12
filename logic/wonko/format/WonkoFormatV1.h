#pragma once

#include "WonkoFormat.h"

class WonkoFormatV1 : public WonkoFormat
{
public:
	BaseWonkoEntity::Ptr parseIndexInternal(const QJsonObject &obj) const override;
	BaseWonkoEntity::Ptr parseVersionInternal(const QJsonObject &obj) const override;
	BaseWonkoEntity::Ptr parseVersionListInternal(const QJsonObject &obj) const override;

	QJsonObject serializeIndexInternal(const WonkoIndex *ptr) const override;
	QJsonObject serializeVersionInternal(const WonkoVersion *ptr) const override;
	QJsonObject serializeVersionListInternal(const WonkoVersionList *ptr) const override;
};
