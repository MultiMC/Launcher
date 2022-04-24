#include "XboxUserStep.h"

#include <QNetworkRequest>

#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"

XboxUserStep::XboxUserStep(AccountData* data) : AuthStep(data) {

}

XboxUserStep::~XboxUserStep() noexcept = default;

QString XboxUserStep::describe() {
    return tr("Logging in as an Xbox user.");
}


void XboxUserStep::rehydrate() {
    // NOOP, for now. We only save bools and there's nothing to check.
}

void XboxUserStep::perform() {
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
    auto xbox_auth_data = xbox_auth_template.arg(m_data->msaToken.token);

    QNetworkRequest request = QNetworkRequest(QUrl("https://user.auth.xboxlive.com/user/authenticate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    auto *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &XboxUserStep::onRequestDone);
    requestor->post(request, xbox_auth_data.toUtf8());
    qDebug() << "First layer of XBox auth ... commencing.";
}

void XboxUserStep::onRequestDone(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    auto requestor = qobject_cast<AuthRequest *>(QObject::sender());
    requestor->deleteLater();

    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Xbox user authentication failed."));
        return;
    }

    Katabasis::Token temp;
    if(!Parsers::parseXTokenResponse(data, temp, "UToken")) {
        qWarning() << "Could not parse user authentication response...";
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Unexpected Xbox user authentication response."));
        return;
    }
    m_data->userToken = temp;
    emit finished(AccountTaskState::STATE_WORKING, tr("Got Xbox user token"));
}
