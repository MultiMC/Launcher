#include "JarMod.h"
#include "Json.h"

using namespace Json;

JarmodPtr Jarmod::fromJson(const QJsonObject &libObj, const QString &filename, const QString &originalName)
{
	JarmodPtr out(new Jarmod());
	if (!libObj.contains("name"))
	{
		throw JSONValidationError(filename +
								  "contains a jarmod that doesn't have a 'name' field");
	}
	out->name = requireString(libObj, "name");
	out->originalName = requireString(libObj, "originalName");
	out->absoluteUrl = ensureString(libObj, "MMC-absoluteUrl");
	return out;
}

QJsonObject Jarmod::toJson()
{
	QJsonObject out;
	writeString(out, "name", name);
	writeString(out, "originalName", originalName);
	writeString(out, "MMC-absoluteUrl", absoluteUrl);
	return out;
}
