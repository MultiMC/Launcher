// Licensed under the Apache-2.0 license. See README.md for details.

#include "Json.h"

#include <QFile>
#include <QRegularExpression>

#include "FileSystem.h"
#include <math.h>
#include <limits>

namespace Json
{
void write(const QJsonDocument &doc, const QString &filename)
{
    FS::write(filename, doc.toJson());
}
void write(const QJsonObject &object, const QString &filename)
{
    write(QJsonDocument(object), filename);
}
void write(const QJsonArray &array, const QString &filename)
{
    write(QJsonDocument(array), filename);
}

QByteArray toBinary(const QJsonObject &obj)
{
    return QJsonDocument(obj).toBinaryData();
}
QByteArray toBinary(const QJsonArray &array)
{
    return QJsonDocument(array).toBinaryData();
}
QByteArray toText(const QJsonObject &obj)
{
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
QByteArray toText(const QJsonArray &array)
{
    return QJsonDocument(array).toJson(QJsonDocument::Compact);
}

static bool isBinaryJson(const QByteArray &data)
{
    decltype(QJsonDocument::BinaryFormatTag) tag = QJsonDocument::BinaryFormatTag;
    if (size_t(data.size()) < sizeof(tag))
    {
        return false;
    }
    return memcmp(data.constData(), &tag, sizeof(QJsonDocument::BinaryFormatTag)) == 0;
}
QJsonDocument requireDocument(const QByteArray &data, const QString &what)
{
    if (isBinaryJson(data))
    {
        QJsonDocument doc = QJsonDocument::fromBinaryData(data);
        if (doc.isNull())
        {
            throw JsonException(what + ": Invalid JSON (binary JSON detected)");
        }
        return doc;
    }
    else
    {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError)
        {
            throw JsonException(what + ": Error parsing JSON: " + error.errorString());
        }
        return doc;
    }
}
QJsonDocument requireDocument(const QString &filename, const QString &what)
{
    return requireDocument(FS::read(filename), what);
}
QJsonObject requireObject(const QJsonDocument &doc, const QString &what)
{
    if (!doc.isObject())
    {
        throw JsonException(what + " is not an object");
    }
    return doc.object();
}
QJsonObject requireObject(const QJsonValueRef &node, const QString &what)
{
    if (!node.isObject())
    {
        throw JsonException(what + " is not an object");
    }
    return node.toObject();
}
QJsonArray requireArray(const QJsonDocument &doc, const QString &what)
{
    if (!doc.isArray())
    {
        throw JsonException(what + " is not an array");
    }
    return doc.array();
}

void writeString(QJsonObject &to, const QString &key, const QString &value)
{
    if (!value.isEmpty())
    {
        to.insert(key, value);
    }
}

void writeStringList(QJsonObject &to, const QString &key, const QStringList &values)
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

template<>
QJsonValue toJson<QUrl>(const QUrl &url)
{
    return QJsonValue(url.toString(QUrl::FullyEncoded));
}
template<>
QJsonValue toJson<QByteArray>(const QByteArray &data)
{
    return QJsonValue(QString::fromLatin1(data.toHex()));
}
template<>
QJsonValue toJson<QDateTime>(const QDateTime &datetime)
{
    return QJsonValue(datetime.toString(Qt::ISODate));
}
template<>
QJsonValue toJson<QDir>(const QDir &dir)
{
    return QDir::current().relativeFilePath(dir.absolutePath());
}
template<>
QJsonValue toJson<QUuid>(const QUuid &uuid)
{
    return uuid.toString();
}
template<>
QJsonValue toJson<QVariant>(const QVariant &variant)
{
    return QJsonValue::fromVariant(variant);
}


template<> QByteArray requireIsType<QByteArray>(const QJsonValue &value, const QString &what)
{
    const QString string = requireIsType<QString>(value, what);
    // ensure that the string can be safely cast to Latin1
    if (string != QString::fromLatin1(string.toLatin1()))
    {
        throw JsonException(what + " is not encodable as Latin1");
    }
    return QByteArray::fromHex(string.toLatin1());
}

template<> QJsonArray requireIsType<QJsonArray>(const QJsonValue &value, const QString &what)
{
    if (!value.isArray())
    {
        throw JsonException(what + " is not an array");
    }
    return value.toArray();
}


template<> QString requireIsType<QString>(const QJsonValue &value, const QString &what)
{
    if (!value.isString())
    {
        throw JsonException(what + " is not a string");
    }
    return value.toString();
}

template<> bool requireIsType<bool>(const QJsonValue &value, const QString &what)
{
    if (!value.isBool())
    {
        throw JsonException(what + " is not a bool");
    }
    return value.toBool();
}

template<> double requireIsType<double>(const QJsonValue &value, const QString &what)
{
    if (!value.isDouble())
    {
        throw JsonException(what + " is not a double");
    }
    return value.toDouble();
}

template<> int requireIsType<int>(const QJsonValue &value, const QString &what)
{
    const double doubl = requireIsType<double>(value, what);
    if (fmod(doubl, 1) != 0)
    {
        throw JsonException(what + " is not an integer");
    }
    if (doubl < std::numeric_limits<int>::min() || doubl > std::numeric_limits<int>::max())
    {
        throw JsonException(what + " is out of range");
    }
    return int(doubl);
}

template<> QDateTime requireIsType<QDateTime>(const QJsonValue &value, const QString &what)
{
    const QString string = requireIsType<QString>(value, what);
    const QDateTime datetime = QDateTime::fromString(string, Qt::ISODate);
    if (!datetime.isValid())
    {
        throw JsonException(what + " is not a ISO formatted date/time value");
    }
    return datetime;
}

template<> QUrl requireIsType<QUrl>(const QJsonValue &value, const QString &what)
{
    const QString string = requireIsType<QString>(value, what);
    if (string.isEmpty())
    {
        throw JsonException(what + " is an empty URL");
    }
    const QUrl url = QUrl(string, QUrl::StrictMode);
    if (!url.isValid())
    {
        throw JsonException(what + " is not a correctly formatted URL");
    }
    return url;
}

template<> QDir requireIsType<QDir>(const QJsonValue &value, const QString &what)
{
    const QString string = requireIsType<QString>(value, what);
    // FIXME: does not handle invalid characters!
    return QDir::current().absoluteFilePath(string);
}

template<> QUuid requireIsType<QUuid>(const QJsonValue &value, const QString &what)
{
    const QString string = requireIsType<QString>(value, what);
    const QUuid uuid = QUuid(string);
    if (uuid.toString() != string) // converts back => valid
    {
        throw JsonException(what + " is not a valid UUID");
    }
    return uuid;
}

template<> QJsonObject requireIsType<QJsonObject>(const QJsonValue &value, const QString &what)
{
    if (!value.isObject())
    {
        throw JsonException(what + " is not an object");
    }
    return value.toObject();
}

template<> QVariant requireIsType<QVariant>(const QJsonValue &value, const QString &what)
{
    if (value.isNull() || value.isUndefined())
    {
        throw JsonException(what + " is null or undefined");
    }
    return value.toVariant();
}

template<> QJsonValue requireIsType<QJsonValue>(const QJsonValue &value, const QString &what)
{
    if (value.isNull() || value.isUndefined())
    {
        throw JsonException(what + " is null or undefined");
    }
    return value;
}

static QJsonValue requireKey(const QJsonObject &parent, const QString &key, const QString &what, QString &localWhat)
{
    localWhat = QString(what).replace("__placeholder__", '\'' + key + '\'');
    if (!parent.contains(key))
    {
        throw JsonException(localWhat + "s parent does not contain " + localWhat);
    }
    return parent.value(key);
}

QJsonDocument requireTextDocument(const QByteArray &data, const QString &what)
{
    if (isBinaryJson(data))
    {
        throw JsonException(what + " is binary JSON");
    }
    if (data.startsWith("\xEF\xBB\xBF"))
    {
        throw JsonException(what + " starts with a byte order mark");
    }
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        throw JsonException(what + ": Error parsing JSON: " + error.errorString());
    }
    return doc;
}

void requireExactKeys(const QJsonObject &object, const QStringList &keys, const QString &what)
{
    for (const auto &key : keys)
    {
        if (!object.contains(key))
        {
            throw JsonException(what + " does not contain '" + key + "'");
        }
    }
    for (auto iter = object.begin(); iter != object.end(); iter++)
    {
        if (!keys.contains(iter.key()))
        {
            throw JsonException(what + " contains an unknown key '" + iter.key() + "'");
        }
    }
}

void requireKeys(const QJsonObject &object, const QStringList &keys, const QString &what)
{
    for (const auto &key : keys)
    {
        if (!object.contains(key))
        {
            throw JsonException(what + " does not contain '" + key + "'");
        }
    }
}

qint64 requireValueBoundedInteger(const QJsonValue &value, qint64 minimum, qint64 maximum, const QString &what)
{
    const double doubl = requireIsType<double>(value, what);
    if (fmod(doubl, 1) != 0)
    {
        throw JsonException(what + " is not an integer");
    }
    const qint64 lowest = qMax(minimum, -MaxExactInteger);
    const qint64 highest = qMin(maximum, MaxExactInteger);
    if (doubl < double(lowest) || doubl > double(highest))
    {
        throw JsonException(QString("%1 is not from %2 to %3").arg(what).arg(lowest).arg(highest));
    }
    return qint64(doubl);
}
qint64 requireBoundedInteger(const QJsonObject &parent, const QString &key, qint64 minimum, qint64 maximum, const QString &what)
{
    QString localWhat;
    const auto value = requireKey(parent, key, what, localWhat);
    return requireValueBoundedInteger(value, minimum, maximum, localWhat);
}

QByteArray requireValueHex(const QJsonValue &value, int bytes, const QString &what)
{
    const QString string = requireIsType<QString>(value, what);
    if (string.size() != bytes * 2)
    {
        throw JsonException(QString("%1 is not %2 hex digits").arg(what).arg(bytes * 2));
    }
    for (const auto c : string)
    {
        if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f'))
        {
            throw JsonException(what + " is not lowercase hex digits");
        }
    }
    return QByteArray::fromHex(string.toLatin1());
}
QByteArray requireHex(const QJsonObject &parent, const QString &key, int bytes, const QString &what)
{
    QString localWhat;
    const auto value = requireKey(parent, key, what, localWhat);
    return requireValueHex(value, bytes, localWhat);
}

QDateTime requireValueUtcTime(const QJsonValue &value, const QString &what)
{
    static const QString format = "yyyy-MM-dd'T'HH:mm:ss'Z'";
    const QString string = requireIsType<QString>(value, what);
    QDateTime datetime = QDateTime::fromString(string, format);
    datetime.setTimeSpec(Qt::UTC);
    // anything that doesn't come back the same, like missing leading zeros, isn't exactly the format
    if (!datetime.isValid() || datetime.toString(format) != string)
    {
        throw JsonException(what + " is not a UTC time written as YYYY-MM-DDTHH:MM:SSZ");
    }
    return datetime;
}
QDateTime requireUtcTime(const QJsonObject &parent, const QString &key, const QString &what)
{
    QString localWhat;
    const auto value = requireKey(parent, key, what, localWhat);
    return requireValueUtcTime(value, localWhat);
}

QString requireValueMatching(const QJsonValue &value, const QString &pattern, const QString &what)
{
    const QString string = requireIsType<QString>(value, what);
    const QRegularExpression expression("\\A(?:" + pattern + ")\\z");
    if (!expression.isValid())
    {
        throw JsonException(what + " can't be checked, the pattern is invalid: " + pattern);
    }
    if (!expression.match(string).hasMatch())
    {
        throw JsonException(what + " is not in the expected form");
    }
    return string;
}
QString requireMatching(const QJsonObject &parent, const QString &key, const QString &pattern, const QString &what)
{
    QString localWhat;
    const auto value = requireKey(parent, key, what, localWhat);
    return requireValueMatching(value, pattern, localWhat);
}

}
