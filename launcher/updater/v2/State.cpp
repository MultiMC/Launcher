#include "State.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

#include "FileSystem.h"
#include "Json.h"

namespace UpdaterV2
{

namespace {
// the version a keys document says it is, to parse it against (only for documents trusted already)
qint64 ownVersion(const QByteArray &data)
{
    auto version = QJsonDocument::fromJson(data).object().value("version");
    return version.isDouble() ? qint64(version.toDouble()) : 0;
}
}

State::State(const QByteArray &floorKeys, const QStringList &pinnedRootKeys, qint64 serialFloor)
{
    QString error;
    if(!parseKeys(floorKeys, ownVersion(floorKeys), m_floorKeys, error))
    {
        m_error = QString("the compiled-in keys document isn't valid: %1").arg(error);
        return;
    }
    if(QSet<QString>::fromList(m_floorKeys.rootKeys) != QSet<QString>::fromList(pinnedRootKeys) || m_floorKeys.rootKeys.size() != pinnedRootKeys.size())
    {
        m_error = "the compiled-in keys document doesn't list exactly the pinned root keys";
        return;
    }
    m_pinnedRootKeys = pinnedRootKeys;
    m_pinnedRootKeys.sort();
    m_serialFloor = serialFloor;
    m_keys = m_floorKeys;
    m_keysData = floorKeys;
    m_serial = serialFloor;
}

void State::load(const QString &path)
{
    if(!m_error.isEmpty())
    {
        return;
    }
    QFile file(path);
    if(!file.exists())
    {
        return;
    }
    if(!file.open(QFile::ReadOnly))
    {
        qWarning() << "Updater state" << path << "can't be read, using the compiled-in floors";
        return;
    }
    QJsonObject obj;
    try
    {
        obj = Json::requireObject(Json::requireTextDocument(file.readAll(), path));
        Json::requireBoundedInteger(obj, "format", 1, 1, "format");
    }
    catch(const Json::JsonException &e)
    {
        qWarning() << "Updater state" << path << "isn't valid, using the compiled-in floors:" << e.cause();
        return;
    }
    QStringList storedRootKeys;
    for(auto key: obj.value("root_keys").toArray())
    {
        storedRootKeys.append(key.toString());
    }
    storedRootKeys.sort();
    if(storedRootKeys != m_pinnedRootKeys)
    {
        qWarning() << "Updater state" << path << "was written by a build pinned to other root keys, using the compiled-in floors";
        return;
    }

    // each part on its own: a broken one leaves its floor
    try
    {
        acceptSerial(Json::requireBoundedInteger(obj, "serial", 1, Json::MaxExactInteger, "serial"));
    }
    catch(const Json::JsonException &e)
    {
        qWarning() << "Updater state: the stored serial isn't valid:" << e.cause();
    }
    auto keysData = obj.value("keys").toString().toUtf8();
    KeysDocument keys;
    QString error;
    if(!keysData.isEmpty())
    {
        if(parseKeys(keysData, ownVersion(keysData), keys, error))
        {
            acceptKeys(keys, keysData);
        }
        else
        {
            qWarning() << "Updater state: the stored keys document isn't valid:" << error;
        }
    }
    auto mirrorsData = obj.value("mirrors").toString().toUtf8();
    MirrorsDocument mirrors;
    if(!mirrorsData.isEmpty())
    {
        if(parseMirrors(mirrorsData, mirrors, error))
        {
            acceptMirrors(mirrors, mirrorsData);
        }
        else
        {
            qWarning() << "Updater state: the stored mirror list isn't valid:" << error;
        }
    }
}

bool State::save(const QString &path) const
{
    if(!m_error.isEmpty())
    {
        return false;
    }
    QJsonObject obj;
    obj.insert("format", 1);
    obj.insert("root_keys", QJsonArray::fromStringList(m_pinnedRootKeys));
    obj.insert("serial", double(m_serial));
    obj.insert("keys", QString::fromUtf8(m_keysData));
    if(hasMirrors())
    {
        obj.insert("mirrors", QString::fromUtf8(m_mirrorsData));
    }
    if(!FS::ensureFilePathExists(path))
    {
        return false;
    }
    QSaveFile file(path);
    if(!file.open(QFile::WriteOnly))
    {
        return false;
    }
    file.write(QJsonDocument(obj).toJson());
    return file.commit();
}

bool State::acceptKeys(const KeysDocument &keys, const QByteArray &data)
{
    if(!m_error.isEmpty() || keys.version <= m_keys.version)
    {
        return false;
    }
    m_keys = keys;
    m_keysData = data;
    return true;
}

bool State::acceptSerial(qint64 serial)
{
    if(!m_error.isEmpty() || serial <= m_serial)
    {
        return false;
    }
    m_serial = serial;
    return true;
}

bool State::acceptMirrors(const MirrorsDocument &mirrors, const QByteArray &data)
{
    if(!m_error.isEmpty() || (hasMirrors() && mirrors.issued <= m_mirrors.issued))
    {
        return false;
    }
    m_mirrors = mirrors;
    m_mirrorsData = data;
    return true;
}

}
