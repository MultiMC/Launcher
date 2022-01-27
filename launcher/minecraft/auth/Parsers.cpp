#include "Parsers.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

namespace Parsers {

bool getDateTime(QJsonValue value, QDateTime & out) {
    if(!value.isString()) {
        return false;
    }
    out = QDateTime::fromString(value.toString(), Qt::ISODate);
    return out.isValid();
}

bool getString(QJsonValue value, QString & out) {
    if(!value.isString()) {
        return false;
    }
    out = value.toString();
    return true;
}

bool getNumber(QJsonValue value, double & out) {
    if(!value.isDouble()) {
        return false;
    }
    out = value.toDouble();
    return true;
}

bool getNumber(QJsonValue value, int64_t & out) {
    if(!value.isDouble()) {
        return false;
    }
    out = (int64_t) value.toDouble();
    return true;
}

bool getBool(QJsonValue value, bool & out) {
    if(!value.isBool()) {
        return false;
    }
    out = value.toBool();
    return true;
}

/*
{
   "IssueInstant":"2020-12-07T19:52:08.4463796Z",
   "NotAfter":"2020-12-21T19:52:08.4463796Z",
   "Token":"token",
   "DisplayClaims":{
      "xui":[
         {
            "uhs":"userhash"
         }
      ]
   }
 }
*/
// TODO: handle error responses ...
/*
{
    "Identity":"0",
    "XErr":2148916238,
    "Message":"",
    "Redirect":"https://start.ui.xboxlive.com/AddChildToFamily"
}
// 2148916233 = missing XBox account
// 2148916238 = child account not linked to a family
*/

bool parseXTokenResponse(QByteArray & data, Katabasis::Token &output, QString name) {
    qDebug() << "Parsing" << name <<":";
#ifndef NDEBUG
    qDebug() << data;
#endif
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if(jsonError.error) {
        qWarning() << "Failed to parse response from user.auth.xboxlive.com as JSON: " << jsonError.errorString();
        return false;
    }

    auto obj = doc.object();
    if(!getDateTime(obj.value("IssueInstant"), output.issueInstant)) {
        qWarning() << "User IssueInstant is not a timestamp";
        return false;
    }
    if(!getDateTime(obj.value("NotAfter"), output.notAfter)) {
        qWarning() << "User NotAfter is not a timestamp";
        return false;
    }
    if(!getString(obj.value("Token"), output.token)) {
        qWarning() << "User Token is not a string";
        return false;
    }
    auto arrayVal = obj.value("DisplayClaims").toObject().value("xui");
    if(!arrayVal.isArray()) {
        qWarning() << "Missing xui claims array";
        return false;
    }
    bool foundUHS = false;
    for(auto item: arrayVal.toArray()) {
        if(!item.isObject()) {
            continue;
        }
        auto obj = item.toObject();
        if(obj.contains("uhs")) {
            foundUHS = true;
        } else {
            continue;
        }
        // consume all 'display claims' ... whatever that means
        for(auto iter = obj.begin(); iter != obj.end(); iter++) {
            QString claim;
            if(!getString(obj.value(iter.key()), claim)) {
                qWarning() << "display claim " << iter.key() << " is not a string...";
                return false;
            }
            output.extra[iter.key()] = claim;
        }

        break;
    }
    if(!foundUHS) {
        qWarning() << "Missing uhs";
        return false;
    }
    output.validity = Katabasis::Validity::Certain;
    qDebug() << name << "is valid.";
    return true;
}

bool parseMinecraftProfile(QByteArray & data, MinecraftProfile &output) {
    qDebug() << "Parsing Minecraft profile...";
#ifndef NDEBUG
    qDebug() << data;
#endif

    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if(jsonError.error) {
        qWarning() << "Failed to parse response from user.auth.xboxlive.com as JSON: " << jsonError.errorString();
        return false;
    }

    auto obj = doc.object();
    if(!getString(obj.value("id"), output.id)) {
        qWarning() << "Minecraft profile id is not a string";
        return false;
    }

    if(!getString(obj.value("name"), output.name)) {
        qWarning() << "Minecraft profile name is not a string";
        return false;
    }

    auto skinsArray = obj.value("skins").toArray();
    for(auto skin: skinsArray) {
        auto skinObj = skin.toObject();
        Skin skinOut;
        if(!getString(skinObj.value("id"), skinOut.id)) {
            continue;
        }
        QString state;
        if(!getString(skinObj.value("state"), state)) {
            continue;
        }
        if(state != "ACTIVE") {
            continue;
        }
        if(!getString(skinObj.value("url"), skinOut.url)) {
            continue;
        }
        if(!getString(skinObj.value("variant"), skinOut.variant)) {
            continue;
        }
        // we deal with only the active skin
        output.skin = skinOut;
        break;
    }
    auto capesArray = obj.value("capes").toArray();

    QString currentCape;
    for(auto cape: capesArray) {
        auto capeObj = cape.toObject();
        Cape capeOut;
        if(!getString(capeObj.value("id"), capeOut.id)) {
            continue;
        }
        QString state;
        if(!getString(capeObj.value("state"), state)) {
            continue;
        }
        if(state == "ACTIVE") {
            currentCape = capeOut.id;
        }
        if(!getString(capeObj.value("url"), capeOut.url)) {
            continue;
        }
        if(!getString(capeObj.value("alias"), capeOut.alias)) {
            continue;
        }

        output.capes[capeOut.id] = capeOut;
    }
    output.currentCape = currentCape;
    output.validity = Katabasis::Validity::Certain;
    return true;
}

bool parseMinecraftEntitlements(QByteArray & data, MinecraftEntitlement &output) {
    qDebug() << "Parsing Minecraft entitlements...";
#ifndef NDEBUG
    qDebug() << data;
#endif

    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if(jsonError.error) {
        qWarning() << "Failed to parse response from user.auth.xboxlive.com as JSON: " << jsonError.errorString();
        return false;
    }

    auto obj = doc.object();
    output.canPlayMinecraft = false;
    output.ownsMinecraft = false;

    auto itemsArray = obj.value("items").toArray();
    for(auto item: itemsArray) {
        auto itemObj = item.toObject();
        QString name;
        if(!getString(itemObj.value("name"), name)) {
            continue;
        }
        if(name == "game_minecraft") {
            output.canPlayMinecraft = true;
        }
        if(name == "product_minecraft") {
            output.ownsMinecraft = true;
        }
    }
    output.validity = Katabasis::Validity::Certain;
    return true;
}

bool parseRolloutResponse(QByteArray & data, bool& result) {
    qDebug() << "Parsing Rollout response...";
#ifndef NDEBUG
    qDebug() << data;
#endif

    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if(jsonError.error) {
        qWarning() << "Failed to parse response from https://api.minecraftservices.com/rollout/v1/msamigration as JSON: " << jsonError.errorString();
        return false;
    }

    auto obj = doc.object();
    QString feature;
    if(!getString(obj.value("feature"), feature)) {
        qWarning() << "Rollout feature is not a string";
        return false;
    }
    if(feature != "msamigration") {
        qWarning() << "Rollout feature is not what we expected (msamigration), but is instead \"" << feature << "\"";
        return false;
    }
    if(!getBool(obj.value("rollout"), result)) {
        qWarning() << "Rollout feature is not a string";
        return false;
    }
    return true;
}

bool parseMojangResponse(QByteArray & data, Katabasis::Token &output) {
    QJsonParseError jsonError;
    qDebug() << "Parsing Mojang response...";
#ifndef NDEBUG
    qDebug() << data;
#endif
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if(jsonError.error) {
        qWarning() << "Failed to parse response from api.minecraftservices.com/launcher/login as JSON: " << jsonError.errorString();
        return false;
    }

    auto obj = doc.object();
    double expires_in = 0;
    if(!getNumber(obj.value("expires_in"), expires_in)) {
        qWarning() << "expires_in is not a valid number";
        return false;
    }
    auto currentTime = QDateTime::currentDateTimeUtc();
    output.issueInstant = currentTime;
    output.notAfter = currentTime.addSecs(expires_in);

    QString username;
    if(!getString(obj.value("username"), username)) {
        qWarning() << "username is not valid";
        return false;
    }

    // TODO: it's a JWT... validate it?
    if(!getString(obj.value("access_token"), output.token)) {
        qWarning() << "access_token is not valid";
        return false;
    }
    output.validity = Katabasis::Validity::Certain;
    qDebug() << "Mojang response is valid.";
    return true;
}

}
