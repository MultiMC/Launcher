/* Copyright 2025-2026 Mehmet Samet Duman
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include "tasks/Task.h"
#include "net/NetJob.h"
#include "java/download/JavaRuntime.h"

#include <QUrl>

class JavaDownloadTask : public Task
{
    Q_OBJECT

public:
    explicit JavaDownloadTask(const JavaDownload::RuntimeEntry &runtime, const QString &targetDir,
                              QObject *parent = nullptr);
    virtual ~JavaDownloadTask() = default;

    QString installedJavaPath() const { return m_installedJavaPath; }

protected:
    void executeTask() override;

private slots:
    void downloadFinished();
    void downloadFailed(QString reason);
    void extractArchive();
    void manifestDownloaded();
    void manifestFilesDownloaded();

private:
    void downloadArchive();
    void downloadManifest();
    QString findJavaBinary(const QString &dir) const;

    JavaDownload::RuntimeEntry m_runtime;
    QString m_targetDir;
    QString m_archivePath;
    QString m_installedJavaPath;
    NetJob::Ptr m_downloadJob;
    QByteArray m_manifestData;
    QStringList m_executableFiles;
    QList<QPair<QString, QString>> m_linkEntries;
};
