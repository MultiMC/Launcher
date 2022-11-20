/*
 * Copyright 2022 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QTextDocument>
#include <net/NetJob.h>

namespace Modplatform {

using Callback = std::function<void(QString)>;
struct ImageLoad {
    QString key;
    NetJob::Ptr job;
    QByteArray output;
    Callback handler;
};

class DescriptionDocument: public QTextDocument {
    Q_OBJECT
public:
    DescriptionDocument(const QString &markdown, QObject * parent = nullptr);

signals:
    void layoutUpdateRequired();

protected:
    QVariant loadResource(int type, const QUrl & name) override;

private:
    void downloadFailed(const QString &key);
    void downloadFinished(const QString &key, const QPixmap &out);

    void requestResource(const QUrl &url);

private:
    QMap<QString, ImageLoad *> m_loading;
    QStringList m_failed;
};

}
