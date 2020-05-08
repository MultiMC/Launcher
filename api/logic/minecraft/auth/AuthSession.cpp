#include "AuthSession.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStringList>

QString AuthSession::serializeUserProperties()
{
    QJsonObject userAttrs;
    for (auto key : u.properties.keys())
    {
        auto array = QJsonArray::fromStringList(u.properties.values(key));
        userAttrs.insert(key, array);
    }
    QJsonDocument value(userAttrs);
    return value.toJson(QJsonDocument::Compact);

}

bool AuthSession::MakeOffline(QString offline_playername)
{
    if (status != PlayableOffline && status != PlayableOnline)
    {
        return false;
    }
    session = "-";
    player_name = offline_playername;
    status = PlayableOffline;
    return true;
}

bool AuthSession::MakeCracked(QString offline_playername)
{
    session = "-";
    // Filling session with dummy data
    client_token = "ff64ff64ff64ff64ff64ff64ff64ff64";
    access_token = "ff64ff64ff64ff64ff64ff64ff64ff64";
    // TODO: Fetch actual UUID's from Mojang API so they match with real ones
    uuid = QString(QCryptographicHash::hash(offline_playername.toLocal8Bit(), QCryptographicHash::Md5).toHex());

    player_name = offline_playername;
    status = PlayableOffline;
    return true;
}