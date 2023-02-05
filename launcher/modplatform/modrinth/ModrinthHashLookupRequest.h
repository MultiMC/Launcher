/*
 * Copyright 2023 arthomnix
 *
 * This source is subject to the Microsoft Public License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QFileInfo>
#include <QJsonObject>
#include "net/NetAction.h"

namespace Modrinth
{

struct HashLookupData
{
    QFileInfo fileInfo;
    QString hash;
};

struct HashLookupResponseData
{
    QFileInfo fileInfo;
    bool found;
    QJsonObject fileJson;
};

class HashLookupRequest : public NetAction
{
public:
    using Ptr = shared_qobject_ptr<HashLookupRequest>;

    explicit HashLookupRequest(QList<HashLookupData> hashes, QList<HashLookupResponseData> *output);
    static Ptr make(QList<HashLookupData> hashes, QList<HashLookupResponseData> *output) {
        return Ptr(new HashLookupRequest(hashes, output));
    }

protected slots:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal) override;
    void downloadError(QNetworkReply::NetworkError error) override;
    void downloadFinished() override;
    void downloadReadyRead() override {}

public slots:
    void startImpl() override;

private:
    QList<HashLookupData> m_hashes;
    std::shared_ptr<QList<HashLookupResponseData>> m_output;
    bool finished = true;
};

}