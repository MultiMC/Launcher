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
		obj.insert("downloadType", "direct");
	case Parallel:
		obj.insert("downloadType", "parallel");
	case Sequential:
		obj.insert("downloadType", "sequential");
	case Encoded:
		obj.insert("downloadType", "encoded");
	case Maven:
		obj.insert("downloadType", "maven");
	}
	return obj;
}
