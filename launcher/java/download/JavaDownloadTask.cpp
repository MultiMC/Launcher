/* Copyright 2025-2026 Mehmet Samet Duman
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "JavaDownloadTask.h"

#include <QDir>
#include <QProcess>
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>

#include "Application.h"
#include "FileSystem.h"
#include "Json.h"
#include "net/Download.h"
#include "net/ChecksumValidator.h"
#include "MMCZip.h"

JavaDownloadTask::JavaDownloadTask(const JavaDownload::RuntimeEntry &runtime, const QString &targetDir,
                                   QObject *parent)
    : Task(parent), m_runtime(runtime), m_targetDir(targetDir)
{
}

void JavaDownloadTask::executeTask()
{
    if (m_runtime.downloadType == "manifest") {
        downloadManifest();
    } else {
        downloadArchive();
    }
}

void JavaDownloadTask::downloadArchive()
{
    setStatus(tr("Downloading %1...").arg(m_runtime.name));

    // Determine archive extension and path
    QUrl url(m_runtime.url);
    QString filename = url.fileName();
    m_archivePath = FS::PathCombine(m_targetDir, filename);

    // Create target directory
    if (!FS::ensureFolderPathExists(m_targetDir)) {
        emitFailed(tr("Failed to create target directory: %1").arg(m_targetDir));
        return;
    }

    m_downloadJob = new NetJob(tr("Java download"), APPLICATION->network());

    auto dl = Net::Download::makeFile(m_runtime.url, m_archivePath);

    // Add checksum validation
    if (!m_runtime.checksumHash.isEmpty()) {
        QCryptographicHash::Algorithm algo = QCryptographicHash::Sha256;
        if (m_runtime.checksumType == "sha1")
            algo = QCryptographicHash::Sha1;
        auto validator = new Net::ChecksumValidator(algo,
                                                     QByteArray::fromHex(m_runtime.checksumHash.toLatin1()));
        dl->addValidator(validator);
    }

    m_downloadJob->addNetAction(dl);

    connect(m_downloadJob.get(), &NetJob::succeeded, this, &JavaDownloadTask::downloadFinished);
    connect(m_downloadJob.get(), &NetJob::failed, this, &JavaDownloadTask::downloadFailed);
    connect(m_downloadJob.get(), &NetJob::progress, this, &Task::setProgress);

    m_downloadJob->start();
}

void JavaDownloadTask::downloadFinished()
{
    m_downloadJob.reset();
    extractArchive();
}

void JavaDownloadTask::downloadFailed(QString reason)
{
    m_downloadJob.reset();
    // Clean up partial download
    if (!m_archivePath.isEmpty() && !QFile::remove(m_archivePath))
        qWarning() << "Failed to remove partial download:" << m_archivePath;
    emitFailed(tr("Failed to download Java: %1").arg(reason));
}

void JavaDownloadTask::extractArchive()
{
    setStatus(tr("Extracting %1...").arg(m_runtime.name));

    bool success = false;

    if (m_archivePath.endsWith(".zip")) {
        // Use QuaZip for zip files
        auto result = MMCZip::extractDir(m_archivePath, m_targetDir);
        success = result.has_value();
    } else if (m_archivePath.endsWith(".tar.gz") || m_archivePath.endsWith(".tgz")) {
        // Use system tar for tar.gz files
        QProcess tarProcess;
        tarProcess.setWorkingDirectory(m_targetDir);
        tarProcess.start("tar", QStringList() << "xzf" << m_archivePath);
        tarProcess.waitForFinished(300000); // 5 minute timeout
        success = (tarProcess.exitCode() == 0 && tarProcess.exitStatus() == QProcess::NormalExit);
        if (!success) {
            qWarning() << "tar extraction failed:" << tarProcess.readAllStandardError();
        }
    } else {
        if (!QFile::remove(m_archivePath))
            qWarning() << "Failed to remove archive:" << m_archivePath;
        emitFailed(tr("Unsupported archive format: %1").arg(m_archivePath));
        return;
    }

    // Clean up archive file
    if (!QFile::remove(m_archivePath))
        qWarning() << "Failed to remove archive:" << m_archivePath;

    if (!success) {
        emitFailed(tr("Failed to extract Java archive."));
        return;
    }

    // Fix permissions on extracted files
    QDirIterator it(m_targetDir, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        auto filepath = it.next();
        QFileInfo file(filepath);
        auto permissions = QFile::permissions(filepath);
        if (file.isDir()) {
            permissions |= QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser;
        } else {
            permissions |= QFileDevice::ReadUser | QFileDevice::WriteUser;
        }
        QFile::setPermissions(filepath, permissions);
    }

    // Find the java binary
    m_installedJavaPath = findJavaBinary(m_targetDir);
    if (m_installedJavaPath.isEmpty()) {
        emitFailed(tr("Could not find java binary in extracted archive."));
        return;
    }

    // Make java binary executable
    auto perms = QFile::permissions(m_installedJavaPath)
        | QFileDevice::ExeUser | QFileDevice::ExeGroup | QFileDevice::ExeOther;
    QFile::setPermissions(m_installedJavaPath, perms);

    qDebug() << "Java installed successfully at:" << m_installedJavaPath;
    emitSucceeded();
}

void JavaDownloadTask::downloadManifest()
{
    setStatus(tr("Downloading manifest for %1...").arg(m_runtime.name));

    if (!FS::ensureFolderPathExists(m_targetDir)) {
        emitFailed(tr("Failed to create target directory: %1").arg(m_targetDir));
        return;
    }

    m_downloadJob = new NetJob(tr("Java manifest download"), APPLICATION->network());
    auto dl = Net::Download::makeByteArray(QUrl(m_runtime.url), &m_manifestData);

    if (m_runtime.checksumType == "sha1" && !m_runtime.checksumHash.isEmpty()) {
        dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1,
                         QByteArray::fromHex(m_runtime.checksumHash.toLatin1())));
    }

    m_downloadJob->addNetAction(dl);

    connect(m_downloadJob.get(), &NetJob::succeeded, this, &JavaDownloadTask::manifestDownloaded);
    connect(m_downloadJob.get(), &NetJob::failed, this, &JavaDownloadTask::downloadFailed);

    m_downloadJob->start();
}

void JavaDownloadTask::manifestDownloaded()
{
    m_downloadJob.reset();

    QJsonDocument doc;
    try {
        doc = Json::requireDocument(m_manifestData);
    } catch (const Exception &e) {
        m_manifestData.clear();
        emitFailed(tr("Failed to parse Java manifest: %1").arg(e.cause()));
        return;
    }
    m_manifestData.clear();

    if (!doc.isObject()) {
        emitFailed(tr("Failed to parse Java manifest."));
        return;
    }

    auto files = doc.object()["files"].toObject();
    m_executableFiles.clear();
    m_linkEntries.clear();

    // Create directories first
    for (auto it = files.begin(); it != files.end(); ++it) {
        auto entry = it.value().toObject();
        if (entry["type"].toString() == "directory") {
            QDir().mkpath(FS::PathCombine(m_targetDir, it.key()));
        }
    }

    // Queue file downloads
    setStatus(tr("Downloading %1 files...").arg(m_runtime.name));
    m_downloadJob = new NetJob(tr("Java runtime files"), APPLICATION->network());

    for (auto it = files.begin(); it != files.end(); ++it) {
        auto entry = it.value().toObject();

        if (entry["type"].toString() == "file") {
            auto downloads = entry["downloads"].toObject();
            auto raw = downloads["raw"].toObject();

            QString url = raw["url"].toString();
            QString sha1 = raw["sha1"].toString();
            QString filePath = FS::PathCombine(m_targetDir, it.key());

            // Ensure parent directory exists
            QFileInfo fi(filePath);
            QDir().mkpath(fi.absolutePath());

            auto dl = Net::Download::makeFile(QUrl(url), filePath);
            if (!sha1.isEmpty()) {
                dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1,
                                 QByteArray::fromHex(sha1.toLatin1())));
            }
            m_downloadJob->addNetAction(dl);

            if (entry["executable"].toBool()) {
                m_executableFiles.append(filePath);
            }
        } else if (entry["type"].toString() == "link") {
            m_linkEntries.append({it.key(), entry["target"].toString()});
        }
    }

    connect(m_downloadJob.get(), &NetJob::succeeded, this, &JavaDownloadTask::manifestFilesDownloaded);
    connect(m_downloadJob.get(), &NetJob::failed, this, &JavaDownloadTask::downloadFailed);
    connect(m_downloadJob.get(), &NetJob::progress, this, &Task::setProgress);

    m_downloadJob->start();
}

void JavaDownloadTask::manifestFilesDownloaded()
{
    m_downloadJob.reset();

    // Create symlinks
    for (const auto &link : m_linkEntries) {
        QString linkPath = FS::PathCombine(m_targetDir, link.first);
        QFileInfo fi(linkPath);
        QDir().mkpath(fi.absolutePath());
        QFile::link(link.second, linkPath);
    }

    // Set executable permissions
    for (const auto &path : m_executableFiles) {
        QFile::setPermissions(path,
            QFile::permissions(path) | QFileDevice::ExeUser | QFileDevice::ExeGroup | QFileDevice::ExeOther);
    }

    // Find java binary
    m_installedJavaPath = findJavaBinary(m_targetDir);
    if (m_installedJavaPath.isEmpty()) {
        emitFailed(tr("Could not find java binary in downloaded runtime."));
        return;
    }

    qDebug() << "Java installed successfully at:" << m_installedJavaPath;
    emitSucceeded();
}

QString JavaDownloadTask::findJavaBinary(const QString &dir) const
{
#if defined(Q_OS_WIN)
    QString binaryName = "javaw.exe";
#else
    QString binaryName = "java";
#endif

    // Search for java binary in bin/ subdirectories
    QDirIterator it(dir, QStringList() << binaryName, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString path = it.filePath();
        if (path.contains("/bin/")) {
            return path;
        }
    }

    // Fallback: any match
    QDirIterator it2(dir, QStringList() << binaryName, QDir::Files, QDirIterator::Subdirectories);
    if (it2.hasNext()) {
        it2.next();
        return it2.filePath();
    }

    return QString();
}
