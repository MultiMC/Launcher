#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QMetaEnum>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QUrlQuery>

#include <QPixmap>
#include <QPainter>

#include "MSAContext.h"
#include "katabasis/Globals.h"
#include "katabasis/Requestor.h"
#include "BuildConfig.h"

using OAuth2 = Katabasis::OAuth2;
using Requestor = Katabasis::Requestor;
using Activity = Katabasis::Activity;

MSAContext::MSAContext(QObject *parent) :
    QObject(parent)
{
    mgr = new QNetworkAccessManager(this);

    Katabasis::OAuth2::Options opts;
    opts.scope = "XboxLive.signin offline_access";
    opts.clientIdentifier = BuildConfig.MSA_CLIENT_ID;
    opts.authorizationUrl = "https://login.live.com/oauth20_authorize.srf";
    opts.accessTokenUrl = "https://login.live.com/oauth20_token.srf";
    opts.listenerPorts = {28562, 28563, 28564, 28565, 28566};

    oauth2 = new OAuth2(opts, m_account.msaToken, this, mgr);

    connect(oauth2, &OAuth2::linkingFailed, this, &MSAContext::onLinkingFailed);
    connect(oauth2, &OAuth2::linkingSucceeded, this, &MSAContext::onLinkingSucceeded);
    connect(oauth2, &OAuth2::openBrowser, this, &MSAContext::onOpenBrowser);
    connect(oauth2, &OAuth2::closeBrowser, this, &MSAContext::onCloseBrowser);
    connect(oauth2, &OAuth2::activityChanged, this, &MSAContext::onOAuthActivityChanged);
}

void MSAContext::beginActivity(Activity activity) {
    if(isBusy()) {
        throw 0;
    }
    activity_ = activity;
    emit activityChanged(activity_);
}

void MSAContext::finishActivity() {
    if(!isBusy()) {
        throw 0;
    }
    activity_ = Katabasis::Activity::Idle;
    m_account.validity_ = m_account.minecraftProfile.validity;
    emit activityChanged(activity_);
}

QString MSAContext::gameToken() {
    return m_account.yggdrasilToken.token;
}

QString MSAContext::userId() {
    return m_account.minecraftProfile.id;
}

QString MSAContext::userName() {
    return m_account.minecraftProfile.name;
}

bool MSAContext::silentSignIn() {
    if(isBusy()) {
        return false;
    }
    beginActivity(Activity::Refreshing);
    if(!oauth2->refresh()) {
        finishActivity();
        return false;
    }

    requestsDone = 0;
    xboxProfileSucceeded = false;
    mcAuthSucceeded = false;

    return true;
}

bool MSAContext::signIn() {
    if(isBusy()) {
        return false;
    }

    requestsDone = 0;
    xboxProfileSucceeded = false;
    mcAuthSucceeded = false;

    beginActivity(Activity::LoggingIn);
    oauth2->unlink();
    m_account = AccountData();
    oauth2->link();
    return true;
}

bool MSAContext::signOut() {
    if(isBusy()) {
        return false;
    }
    beginActivity(Activity::LoggingOut);
    oauth2->unlink();
    m_account = AccountData();
    finishActivity();
    return true;
}


void MSAContext::onOpenBrowser(const QUrl &url) {
    QDesktopServices::openUrl(url);
}

void MSAContext::onCloseBrowser() {

}

void MSAContext::onLinkingFailed() {
    finishActivity();
}

void MSAContext::onLinkingSucceeded() {
    auto *o2t = qobject_cast<OAuth2 *>(sender());
    if (!o2t->linked()) {
        finishActivity();
        return;
    }
    QVariantMap extraTokens = o2t->extraTokens();
    if (!extraTokens.isEmpty()) {
        qDebug() << "Extra tokens in response:";
        foreach (QString key, extraTokens.keys()) {
            qDebug() << "\t" << key << ":" << extraTokens.value(key);
        }
    }
    doUserAuth();
}

void MSAContext::onOAuthActivityChanged(Katabasis::Activity activity) {
    // respond to activity change here
}

void MSAContext::doUserAuth() {
    QString xbox_auth_template = R"XXX(
{
    "Properties": {
        "AuthMethod": "RPS",
        "SiteName": "user.auth.xboxlive.com",
        "RpsTicket": "d=%1"
    },
    "RelyingParty": "http://auth.xboxlive.com",
    "TokenType": "JWT"
}
)XXX";
    auto xbox_auth_data = xbox_auth_template.arg(m_account.msaToken.token);

    QNetworkRequest request = QNetworkRequest(QUrl("https://user.auth.xboxlive.com/user/authenticate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    auto *requestor = new Katabasis::Requestor(mgr, oauth2, this);
    requestor->setAddAccessTokenInQuery(false);

    connect(requestor, &Requestor::finished, this, &MSAContext::onUserAuthDone);
    requestor->post(request, xbox_auth_data.toUtf8());
    qDebug() << "First layer of XBox auth ... commencing.";
}

namespace {
bool getDateTime(QJsonValue value, QDateTime & out) {
    if(!value.isString()) {
        return false;
    }
    out = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
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

bool parseXTokenResponse(QByteArray & data, Katabasis::Token &output) {
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if(jsonError.error) {
        qWarning() << "Failed to parse response from user.auth.xboxlive.com as JSON: " << jsonError.errorString();
        qDebug() << data;
        return false;
    }

    auto obj = doc.object();
    if(!getDateTime(obj.value("IssueInstant"), output.issueInstant)) {
        qWarning() << "User IssueInstant is not a timestamp";
        qDebug() << data;
        return false;
    }
    if(!getDateTime(obj.value("NotAfter"), output.notAfter)) {
        qWarning() << "User NotAfter is not a timestamp";
        qDebug() << data;
        return false;
    }
    if(!getString(obj.value("Token"), output.token)) {
        qWarning() << "User Token is not a timestamp";
        qDebug() << data;
        return false;
    }
    auto arrayVal = obj.value("DisplayClaims").toObject().value("xui");
    if(!arrayVal.isArray()) {
        qWarning() << "Missing xui claims array";
        qDebug() << data;
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
                qDebug() << data;
                return false;
            }
            output.extra[iter.key()] = claim;
        }

        break;
    }
    if(!foundUHS) {
        qWarning() << "Missing uhs";
        qDebug() << data;
        return false;
    }
    output.validity = Katabasis::Validity::Certain;
    qDebug() << data;
    return true;
}

}

void MSAContext::onUserAuthDone(
    int requestId,
    QNetworkReply::NetworkError error,
    QByteArray replyData,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
        finishActivity();
        return;
    }

    Katabasis::Token temp;
    if(!parseXTokenResponse(replyData, temp)) {
        qWarning() << "Could not parse user authentication response...";
        finishActivity();
        return;
    }
    m_account.userToken = temp;

    doSTSAuthMinecraft();
    doSTSAuthGeneric();
}
/*
        url = "https://xsts.auth.xboxlive.com/xsts/authorize"
        headers = {"x-xbl-contract-version": "1"}
        data = {
            "RelyingParty": relying_party,
            "TokenType": "JWT",
            "Properties": {
                "UserTokens": [self.user_token.token],
                "SandboxId": "RETAIL",
            },
        }
*/
void MSAContext::doSTSAuthMinecraft() {
    QString xbox_auth_template = R"XXX(
{
    "Properties": {
        "SandboxId": "RETAIL",
        "UserTokens": [
            "%1"
        ]
    },
    "RelyingParty": "rp://api.minecraftservices.com/",
    "TokenType": "JWT"
}
)XXX";
    auto xbox_auth_data = xbox_auth_template.arg(m_account.userToken.token);

    QNetworkRequest request = QNetworkRequest(QUrl("https://xsts.auth.xboxlive.com/xsts/authorize"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    Requestor *requestor = new Requestor(mgr, oauth2, this);
    requestor->setAddAccessTokenInQuery(false);

    connect(requestor, &Requestor::finished, this, &MSAContext::onSTSAuthMinecraftDone);
    requestor->post(request, xbox_auth_data.toUtf8());
    qDebug() << "Second layer of XBox auth ... commencing.";
}

void MSAContext::onSTSAuthMinecraftDone(
    int requestId,
    QNetworkReply::NetworkError error,
    QByteArray replyData,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
        finishActivity();
        return;
    }

    Katabasis::Token temp;
    if(!parseXTokenResponse(replyData, temp)) {
        qWarning() << "Could not parse authorization response for access to mojang services...";
        finishActivity();
        return;
    }

    if(temp.extra["uhs"] != m_account.userToken.extra["uhs"]) {
        qWarning() << "Server has changed user hash in the reply... something is wrong. ABORTING";
        qDebug() << replyData;
        finishActivity();
        return;
    }
    m_account.mojangservicesToken = temp;

    doMinecraftAuth();
}

void MSAContext::doSTSAuthGeneric() {
    QString xbox_auth_template = R"XXX(
{
    "Properties": {
        "SandboxId": "RETAIL",
        "UserTokens": [
            "%1"
        ]
    },
    "RelyingParty": "http://xboxlive.com",
    "TokenType": "JWT"
}
)XXX";
    auto xbox_auth_data = xbox_auth_template.arg(m_account.userToken.token);

    QNetworkRequest request = QNetworkRequest(QUrl("https://xsts.auth.xboxlive.com/xsts/authorize"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    Requestor *requestor = new Requestor(mgr, oauth2, this);
    requestor->setAddAccessTokenInQuery(false);

    connect(requestor, &Requestor::finished, this, &MSAContext::onSTSAuthGenericDone);
    requestor->post(request, xbox_auth_data.toUtf8());
    qDebug() << "Second layer of XBox auth ... commencing.";
}

void MSAContext::onSTSAuthGenericDone(
    int requestId,
    QNetworkReply::NetworkError error,
    QByteArray replyData,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
        finishActivity();
        return;
    }

    Katabasis::Token temp;
    if(!parseXTokenResponse(replyData, temp)) {
        qWarning() << "Could not parse authorization response for access to xbox API...";
        finishActivity();
        return;
    }

    if(temp.extra["uhs"] != m_account.userToken.extra["uhs"]) {
        qWarning() << "Server has changed user hash in the reply... something is wrong. ABORTING";
        qDebug() << replyData;
        finishActivity();
        return;
    }
    m_account.xboxApiToken = temp;

    doXBoxProfile();
}


void MSAContext::doMinecraftAuth() {
    QString mc_auth_template = R"XXX(
{
    "identityToken": "XBL3.0 x=%1;%2"
}
)XXX";
    auto data = mc_auth_template.arg(m_account.mojangservicesToken.extra["uhs"].toString(), m_account.mojangservicesToken.token);

    QNetworkRequest request = QNetworkRequest(QUrl("https://api.minecraftservices.com/authentication/login_with_xbox"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    Requestor *requestor = new Requestor(mgr, oauth2, this);
    requestor->setAddAccessTokenInQuery(false);

    connect(requestor, &Requestor::finished, this, &MSAContext::onMinecraftAuthDone);
    requestor->post(request, data.toUtf8());
    qDebug() << "Getting Minecraft access token...";
}

namespace {
bool parseMojangResponse(QByteArray & data, Katabasis::Token &output) {
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if(jsonError.error) {
        qWarning() << "Failed to parse response from user.auth.xboxlive.com as JSON: " << jsonError.errorString();
        qDebug() << data;
        return false;
    }

    auto obj = doc.object();
    double expires_in = 0;
    if(!getNumber(obj.value("expires_in"), expires_in)) {
        qWarning() << "expires_in is not a valid number";
        qDebug() << data;
        return false;
    }
    auto currentTime = QDateTime::currentDateTimeUtc();
    output.issueInstant = currentTime;
    output.notAfter = currentTime.addSecs(expires_in);

    QString username;
    if(!getString(obj.value("username"), username)) {
        qWarning() << "username is not valid";
        qDebug() << data;
        return false;
    }

    // TODO: it's a JWT... validate it?
    if(!getString(obj.value("access_token"), output.token)) {
        qWarning() << "access_token is not valid";
        qDebug() << data;
        return false;
    }
    output.validity = Katabasis::Validity::Certain;
    qDebug() << data;
    return true;
}
}

void MSAContext::onMinecraftAuthDone(
    int requestId,
    QNetworkReply::NetworkError error,
    QByteArray replyData,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    requestsDone++;

    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
        qDebug() << replyData;
        finishActivity();
        return;
    }

    if(!parseMojangResponse(replyData, m_account.yggdrasilToken)) {
        qWarning() << "Could not parse login_with_xbox response...";
        qDebug() << replyData;
        finishActivity();
        return;
    }
    mcAuthSucceeded = true;

    checkResult();
}

void MSAContext::doXBoxProfile() {
    auto url = QUrl("https://profile.xboxlive.com/users/me/profile/settings");
    QUrlQuery q;
    q.addQueryItem(
        "settings",
        "GameDisplayName,AppDisplayName,AppDisplayPicRaw,GameDisplayPicRaw,"
        "PublicGamerpic,ShowUserAsAvatar,Gamerscore,Gamertag,ModernGamertag,ModernGamertagSuffix,"
        "UniqueModernGamertag,AccountTier,TenureLevel,XboxOneRep,"
        "PreferredColor,Location,Bio,Watermarks,"
        "RealName,RealNameOverride,IsQuarantined"
    );
    url.setQuery(q);

    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("x-xbl-contract-version", "3");
    request.setRawHeader("Authorization", QString("XBL3.0 x=%1;%2").arg(m_account.userToken.extra["uhs"].toString(), m_account.xboxApiToken.token).toUtf8());
    Requestor *requestor = new Requestor(mgr, oauth2, this);
    requestor->setAddAccessTokenInQuery(false);

    connect(requestor, &Requestor::finished, this, &MSAContext::onXBoxProfileDone);
    requestor->get(request);
    qDebug() << "Getting Xbox profile...";
}

void MSAContext::onXBoxProfileDone(
    int requestId,
    QNetworkReply::NetworkError error,
    QByteArray replyData,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    requestsDone ++;

    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
        qDebug() << replyData;
        finishActivity();
        return;
    }

    qDebug() << "XBox profile: " << replyData;

    xboxProfileSucceeded = true;
    checkResult();
}

void MSAContext::checkResult() {
    if(requestsDone != 2) {
        return;
    }
    if(mcAuthSucceeded && xboxProfileSucceeded) {
        doMinecraftProfile();
    }
    else {
        finishActivity();
    }
}

namespace {
bool parseMinecraftProfile(QByteArray & data, MinecraftProfile &output) {
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if(jsonError.error) {
        qWarning() << "Failed to parse response from user.auth.xboxlive.com as JSON: " << jsonError.errorString();
        qDebug() << data;
        return false;
    }

    auto obj = doc.object();
    if(!getString(obj.value("id"), output.id)) {
        qWarning() << "minecraft profile id is not a string";
        qDebug() << data;
        return false;
    }

    if(!getString(obj.value("name"), output.name)) {
        qWarning() << "minecraft profile name is not a string";
        qDebug() << data;
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
    int i = -1;
    int currentCape = -1;
    for(auto cape: capesArray) {
        i++;
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
            currentCape = i;
        }
        if(!getString(capeObj.value("url"), capeOut.url)) {
            continue;
        }
        if(!getString(capeObj.value("alias"), capeOut.alias)) {
            continue;
        }

        // we deal with only the active skin
        output.capes.push_back(capeOut);
    }
    output.currentCape = currentCape;
    output.validity = Katabasis::Validity::Certain;
    return true;
}
}

void MSAContext::doMinecraftProfile() {
    auto url = QUrl("https://api.minecraftservices.com/minecraft/profile");
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_account.yggdrasilToken.token).toUtf8());

    Requestor *requestor = new Requestor(mgr, oauth2, this);
    requestor->setAddAccessTokenInQuery(false);

    connect(requestor, &Requestor::finished, this, &MSAContext::onMinecraftProfileDone);
    requestor->get(request);
}

void MSAContext::onMinecraftProfileDone(int, QNetworkReply::NetworkError error, QByteArray data, QList<QNetworkReply::RawHeaderPair> headers) {
    qDebug() << data;
    if (error == QNetworkReply::ContentNotFoundError) {
        m_account.minecraftProfile = MinecraftProfile();
        finishActivity();
        return;
    }
    if (error != QNetworkReply::NoError) {
        finishActivity();
        return;
    }
    if(!parseMinecraftProfile(data, m_account.minecraftProfile)) {
        m_account.minecraftProfile = MinecraftProfile();
        finishActivity();
        return;
    }
    doGetSkin();
}

void MSAContext::doGetSkin() {
    auto url = QUrl(m_account.minecraftProfile.skin.url);
    QNetworkRequest request = QNetworkRequest(url);
    Requestor *requestor = new Requestor(mgr, oauth2, this);
    requestor->setAddAccessTokenInQuery(false);
    connect(requestor, &Requestor::finished, this, &MSAContext::onSkinDone);
    requestor->get(request);
}

void MSAContext::onSkinDone(int, QNetworkReply::NetworkError error, QByteArray data, QList<QNetworkReply::RawHeaderPair>) {
    if (error == QNetworkReply::NoError) {
        m_account.minecraftProfile.skin.data = data;
    }
    finishActivity();
}
