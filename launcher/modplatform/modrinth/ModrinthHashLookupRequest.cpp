/*
 * Copyright 2023 arthomnix
 *
 * This source is subject to the Microsoft Public License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include "ModrinthHashLookupRequest.h"
#include "BuildConfig.h"
#include "Json.h"

namespace Modrinth
{

HashLookupRequest::HashLookupRequest(QList<HashLookupData> hashes, QList<HashLookupResponseData> *output) : NetAction(), m_hashes(hashes), m_output(output)
{
    m_url = "https://api.modrinth.com/v2/version_files";
    m_status = Job_NotStarted;
}

void HashLookupRequest::startImpl()
{
    finished = false;
    m_status = Job_InProgress;

    QNetworkRequest request(m_url);
    request.setHeader(QNetworkRequest::UserAgentHeader, BuildConfig.USER_AGENT_UNCACHED);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject requestObject;
    QJsonArray hashes;

    for (const auto &data : m_hashes) {
        hashes.append(data.hash);
    }

    requestObject.insert("hashes", hashes);
    requestObject.insert("algorithm", QJsonValue("sha512"));

    QNetworkReply *rep = m_network->post(request, QJsonDocument(requestObject).toJson());
    m_reply.reset(rep);
    connect(rep, &QNetworkReply::uploadProgress, this, &HashLookupRequest::downloadProgress);
    connect(rep, &QNetworkReply::finished, this, &HashLookupRequest::downloadFinished);
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
}

void HashLookupRequest::downloadError(QNetworkReply::NetworkError error)
{
    qCritical() << "Modrinth hash lookup request failed with error" << m_reply->errorString() << "Server reply:\n" << m_reply->readAll();
    if (finished) {
        qCritical() << "Double finished ModrinthHashLookupRequest!";
        return;
    }
    m_status = Job_Failed;
    finished = true;
    m_reply.reset();
    emit failed(m_index_within_job);
}

void HashLookupRequest::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    m_total_progress = bytesTotal;
    m_progress = bytesReceived;
    emit netActionProgress(m_index_within_job, bytesReceived, bytesTotal);
}

void HashLookupRequest::downloadFinished()
{
    if (finished) {
        qCritical() << "Double finished ModrinthHashLookupRequest!";
        return;
    }

    QByteArray data = m_reply->readAll();
    m_reply.reset();

    try {
        auto document = Json::requireDocument(data);
        auto rootObject = Json::requireObject(document);

        for (const auto &hashData : m_hashes) {
            if (rootObject.contains(hashData.hash)) {
                auto versionObject = Json::requireObject(rootObject, hashData.hash);

                auto files = Json::requireIsArrayOf<QJsonObject>(versionObject, "files");

                QJsonObject file;

                for (const auto &fileJson : files) {
                    auto hashes = Json::requireObject(fileJson, "hashes");
                    QString sha512 = Json::requireString(hashes, "sha512");

                    if (sha512 == hashData.hash) {
                        file = fileJson;
                    }
                }

                m_output->append(HashLookupResponseData {
                    hashData.fileInfo,
                    true,
                    file
                });
            } else {
                m_output->append(HashLookupResponseData {
                    hashData.fileInfo,
                    false,
                    QJsonObject()
                });
            }
        }

        m_status = Job_Finished;
        finished = true;
        emit succeeded(m_index_within_job);
    } catch (const Json::JsonException &e) {
        qCritical() << "Failed to parse Modrinth hash lookup response: " << e.cause();
        m_status = Job_Failed;
        finished = true;
        emit failed(m_index_within_job);
    }
}
}