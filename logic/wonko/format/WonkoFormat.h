#pragma once

#include <QJsonObject>
#include <memory>

#include "Exception.h"
#include "wonko/BaseWonkoEntity.h"

class WonkoIndex;
class WonkoVersion;
class WonkoVersionList;

class WonkoParseException : public Exception
{
public:
	using Exception::Exception;
};

class WonkoFormat
{
public:
	virtual ~WonkoFormat() {}

	static void parseIndex(const QJsonObject &obj, WonkoIndex *ptr);
	static void parseVersion(const QJsonObject &obj, WonkoVersion *ptr);
	static void parseVersionList(const QJsonObject &obj, WonkoVersionList *ptr);

	static QJsonObject serializeIndex(const WonkoIndex *ptr);
	static QJsonObject serializeVersion(const WonkoVersion *ptr);
	static QJsonObject serializeVersionList(const WonkoVersionList *ptr);

protected:
	virtual BaseWonkoEntity::Ptr parseIndexInternal(const QJsonObject &obj) const = 0;
	virtual BaseWonkoEntity::Ptr parseVersionInternal(const QJsonObject &obj) const = 0;
	virtual BaseWonkoEntity::Ptr parseVersionListInternal(const QJsonObject &obj) const = 0;
	virtual QJsonObject serializeIndexInternal(const WonkoIndex *ptr) const = 0;
	virtual QJsonObject serializeVersionInternal(const WonkoVersion *ptr) const = 0;
	virtual QJsonObject serializeVersionListInternal(const WonkoVersionList *ptr) const = 0;
};
