#include "ForcedMigrationStep.h"

#include <QNetworkRequest>

#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"

ForcedMigrationStep::ForcedMigrationStep(AccountData* data) : AuthStep(data) {

}

ForcedMigrationStep::~ForcedMigrationStep() noexcept = default;

QString ForcedMigrationStep::describe() {
    return tr("Checking for migration eligibility.");
}

void ForcedMigrationStep::perform() {
    auto url = QUrl("https://api.minecraftservices.com/rollout/v1/msamigrationforced");
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_data->yggdrasilToken.token).toUtf8());

    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &ForcedMigrationStep::onRequestDone);
    requestor->get(request);
}

void ForcedMigrationStep::rehydrate() {
    // NOOP, for now. We only save bools and there's nothing to check.
}

void ForcedMigrationStep::onRequestDone(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    auto requestor = qobject_cast<AuthRequest *>(QObject::sender());
    requestor->deleteLater();

    if (error == QNetworkReply::NoError) {
        Parsers::parseForcedMigrationResponse(data, m_data->mustMigrateToMSA);
    }
    if(m_data->mustMigrateToMSA) {
        emit finished(AccountTaskState::STATE_FAILED_MUST_MIGRATE, tr("The account must be migrated to a Microsoft account."));
    }
    else {
        emit finished(AccountTaskState::STATE_WORKING, tr("Got forced migration flags"));
    }

}

