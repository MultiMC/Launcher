#include "MMCJson.h"

#include <QString>
#include <QUrl>
#include <QStringList>
#include <math.h>

static bool isBinaryJson(const QByteArray &data)
{
	decltype(QJsonDocument::BinaryFormatTag) tag = QJsonDocument::BinaryFormatTag;
	return memcmp(data.constData(), &tag, sizeof(QJsonDocument::BinaryFormatTag)) == 0;
}
QJsonDocument MMCJson::parseDocument(const QByteArray &data, const QString &what)
{
	if (isBinaryJson(data))
	{
		QJsonDocument doc = QJsonDocument::fromBinaryData(data);
		if (doc.isNull())
		{
			throw JSONValidationError(what + " is not valid JSON (binary JSON detected)");
		}
		return doc;
	}
	else
	{
		QJsonParseError error;
		QJsonDocument doc = QJsonDocument::fromJson(data, &error);
		if (error.error != QJsonParseError::NoError)
		{
			throw JSONValidationError(what + " is not valid JSON: " + error.errorString() + " at " + error.offset);
		}
		return doc;
	}
}

bool MMCJson::ensureBoolean(const QJsonValue val, const QString what)
{
	if (!val.isBool())
		throw JSONValidationError(what + " is not boolean");
	return val.toBool();
}

QJsonValue MMCJson::ensureExists(QJsonValue val, const QString what)
{
	if(val.isUndefined() || val.isUndefined())
		throw JSONValidationError(what + " does not exist");
	return val;
}

QJsonArray MMCJson::ensureArray(const QJsonValue val, const QString what)
{
	if (!val.isArray())
		throw JSONValidationError(what + " is not an array");
	return val.toArray();
}

QJsonArray MMCJson::ensureArray(const QJsonDocument &val, const QString &what)
{
	if (!val.isArray())
	{
		throw JSONValidationError(what + " is not an array");
	}
	return val.array();
}

double MMCJson::ensureDouble(const QJsonValue val, const QString what)
{
	if (!val.isDouble())
		throw JSONValidationError(what + " is not a number");
	return val.toDouble();
}

int MMCJson::ensureInteger(const QJsonValue val, const QString what)
{
	double ret = ensureDouble(val, what);
	if (fmod(ret, 1) != 0)
		throw JSONValidationError(what + " is not an integer");
	return ret;
}

QJsonObject MMCJson::ensureObject(const QJsonValue val, const QString what)
{
	if (!val.isObject())
		throw JSONValidationError(what + " is not an object");
	return val.toObject();
}

QJsonObject MMCJson::ensureObject(const QJsonDocument val, const QString what)
{
	if (!val.isObject())
		throw JSONValidationError(what + " is not an object");
	return val.object();
}

QString MMCJson::ensureString(const QJsonValue val, const QString what)
{
	if (!val.isString())
		throw JSONValidationError(what + " is not a string");
	return val.toString();
}

QUrl MMCJson::ensureUrl(const QJsonValue &val, const QString &what)
{
	const QUrl url = QUrl(ensureString(val, what));
	if (!url.isValid())
	{
		throw JSONValidationError(what + " is not an url");
	}
	return url;
}

QJsonDocument MMCJson::parseFile(const QString &filename, const QString &what)
{
	QFile f(filename);
	if (!f.open(QFile::ReadOnly))
	{
		throw FileOpenError(f);
	}
	return parseDocument(f.readAll(), what);
}
void MMCJson::writeFile(const QString &filename, const QJsonDocument &doc, const bool binary)
{
	QFile f(filename);
	if (!f.open(QFile::WriteOnly))
	{
		throw FileOpenError(f);
	}
	const QByteArray data = binary ? doc.toBinaryData() : doc.toJson();
	if (data.size() != f.write(data))
	{
		throw MMCError(QObject::tr("Unable to write to %1: %2").arg(f.fileName(), f.errorString()));
	}
}

int MMCJson::ensureInteger(const QJsonValue val, QString what, const int def)
{
	if (val.isUndefined())
		return def;
	return ensureInteger(val, what);
}

void MMCJson::writeString(QJsonObject &to, QString key, QString value)
{
	if (!value.isEmpty())
	{
		to.insert(key, value);
	}
}

void MMCJson::writeStringList(QJsonObject &to, QString key, QStringList values)
{
	if (!values.isEmpty())
	{
		QJsonArray array;
		for(auto value: values)
		{
			array.append(value);
		}
		to.insert(key, array);
	}
}

QStringList MMCJson::ensureStringList(const QJsonValue val, QString what)
{
	const QJsonArray array = ensureArray(val, what);
	QStringList out;
	for (const auto value : array)
	{
		out.append(ensureString(value));
	}
	return out;
}
