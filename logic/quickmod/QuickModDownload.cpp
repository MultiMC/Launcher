#include "QuickModDownload.h"
#include "logic/MMCJson.h"

QJsonObject QuickModDownload::toJson() const
{
	QJsonObject obj;
	obj.insert("url", url);
	obj.insert("priority", priority);
	MMCJson::writeString(obj, "hint", hint);
	MMCJson::writeString(obj, "group", group);
	switch (type)
	{
	case Direct:
		obj.insert("downloadType", QStringLiteral("direct"));
	case Parallel:
		obj.insert("downloadType", QStringLiteral("parallel"));
	case Sequential:
		obj.insert("downloadType", QStringLiteral("sequential"));
	case Encoded:
		obj.insert("downloadType", QStringLiteral("encoded"));
	}
	return obj;
}
