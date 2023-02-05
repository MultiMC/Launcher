/*
 * Copyright 2023 arthomnix
 *
 * This source is subject to the Microsoft Public License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include <QDir>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QMap>
#include "Json.h"
#include "ModrinthInstanceExportTask.h"
#include "net/NetJob.h"
#include "Application.h"
#include "ui/dialogs/ModrinthExportDialog.h"
#include "JlCompress.h"
#include "FileSystem.h"

namespace Modrinth
{

InstanceExportTask::InstanceExportTask(InstancePtr instance, ExportSettings settings) : m_instance(instance), m_settings(settings) {}

void InstanceExportTask::executeTask()
{
    setStatus(tr("Finding files to look up on Modrinth..."));

    QDir modsDir(m_instance->gameRoot() + "/mods");
    modsDir.setFilter(QDir::Files);
    modsDir.setNameFilters(QStringList() << "*.jar");

    QDir resourcePacksDir(m_instance->gameRoot() + "/resourcepacks");
    resourcePacksDir.setFilter(QDir::Files);
    resourcePacksDir.setNameFilters(QStringList() << "*.zip");

    QDir shaderPacksDir(m_instance->gameRoot() + "/shaderpacks");
    shaderPacksDir.setFilter(QDir::Files);
    shaderPacksDir.setNameFilters(QStringList() << "*.zip");

    QStringList filesToResolve;

    if (modsDir.exists()) {
        QDirIterator modsIterator(modsDir);
        while (modsIterator.hasNext()) {
            filesToResolve << modsIterator.next();
        }
    }

    if (m_settings.includeResourcePacks && resourcePacksDir.exists()) {
        QDirIterator resourcePacksIterator(resourcePacksDir);
        while (resourcePacksIterator.hasNext()) {
            filesToResolve << resourcePacksIterator.next();
        }
    }

    if (m_settings.includeShaderPacks && shaderPacksDir.exists()) {
        QDirIterator shaderPacksIterator(shaderPacksDir);
        while (shaderPacksIterator.hasNext()) {
            filesToResolve << shaderPacksIterator.next();
        }
    }

    if (!m_settings.datapacksPath.isEmpty()) {
        QDir datapacksDir(m_instance->gameRoot() + "/" + m_settings.datapacksPath);
        datapacksDir.setFilter(QDir::Files);
        datapacksDir.setNameFilters(QStringList() << "*.zip");

        if (datapacksDir.exists()) {
            QDirIterator datapacksIterator(datapacksDir);
            while (datapacksIterator.hasNext()) {
                filesToResolve << datapacksIterator.next();
            }
        }
    }

    m_netJob = new NetJob(tr("Modrinth pack export"), APPLICATION->network());

    for (const QString &filePath: filesToResolve) {
        qDebug() << "Attempting to resolve file hash from Modrinth API: " << filePath;
        QFile file(filePath);

        if (file.open(QFile::ReadOnly)) {
            QByteArray contents = file.readAll();
            QCryptographicHash hasher(QCryptographicHash::Sha512);
            hasher.addData(contents);
            QString hash = hasher.result().toHex();

            m_responses.append(HashLookupData{
                    QFileInfo(file),
                    QByteArray()
            });

            m_netJob->addNetAction(Net::Download::makeByteArray(
                    QString("https://api.modrinth.com/v2/version_file/%1?algorithm=sha512").arg(hash),
                    &m_responses.last().response,
                    Net::Download::Options(Net::Download::Option::AllowNotFound)
            ));
        }
    }

    connect(m_netJob.get(), &NetJob::succeeded, this, &InstanceExportTask::lookupSucceeded);
    connect(m_netJob.get(), &NetJob::failed, this, &InstanceExportTask::lookupFailed);
    connect(m_netJob.get(), &NetJob::progress, this, &InstanceExportTask::lookupProgress);

    m_netJob->start();
    setStatus(tr("Looking up files on Modrinth..."));
}

void InstanceExportTask::lookupSucceeded()
{
    setStatus(tr("Creating modpack metadata..."));
    QList<ExportFile> resolvedFiles;
    QFileInfoList failedFiles;

    for (const auto &data : m_responses) {
        try {
            auto document = Json::requireDocument(data.response);
            auto object = Json::requireObject(document);
            auto file = Json::requireIsArrayOf<QJsonObject>(object, "files").first();
            auto url = Json::requireString(file, "url");
            auto hashes = Json::requireObject(file, "hashes");

            QString sha512Hash = Json::requireString(hashes, "sha512");
            QString sha1Hash = Json::requireString(hashes, "sha1");

            ExportFile fileData;

            QDir gameDir(m_instance->gameRoot());

            fileData.path = gameDir.relativeFilePath(data.fileInfo.absoluteFilePath());
            fileData.download = url;
            fileData.sha512 = sha512Hash;
            fileData.sha1 = sha1Hash;
            fileData.fileSize = data.fileInfo.size();

            resolvedFiles << fileData;
        } catch (const Json::JsonException &e) {
            qDebug() << "File " << data.fileInfo.absoluteFilePath() << " failed to process for reason " << e.cause() << ", adding to overrides";
            failedFiles << data.fileInfo;
        }
    }

    QJsonObject indexJson;
    indexJson.insert("formatVersion", QJsonValue(1));
    indexJson.insert("game", QJsonValue("minecraft"));
    indexJson.insert("versionId", QJsonValue(m_settings.version));
    indexJson.insert("name", QJsonValue(m_settings.name));

    if (!m_settings.description.isEmpty()) {
        indexJson.insert("summary", QJsonValue(m_settings.description));
    }

    QJsonArray files;

    for (const auto &file : resolvedFiles) {
        QJsonObject fileObj;
        fileObj.insert("path", file.path);

        QJsonObject hashes;
        hashes.insert("sha512", file.sha512);
        hashes.insert("sha1", file.sha1);
        fileObj.insert("hashes", hashes);

        QJsonArray downloads;
        downloads.append(file.download);
        fileObj.insert("downloads", downloads);

        fileObj.insert("fileSize", QJsonValue(file.fileSize));

        files.append(fileObj);
    }

    indexJson.insert("files", files);

    QJsonObject dependencies;
    dependencies.insert("minecraft", m_settings.gameVersion);
    if (!m_settings.forgeVersion.isEmpty()) {
        dependencies.insert("forge", m_settings.forgeVersion);
    }
    if (!m_settings.fabricVersion.isEmpty()) {
        dependencies.insert("fabric-loader", m_settings.fabricVersion);
    }
    if (!m_settings.quiltVersion.isEmpty()) {
        dependencies.insert("quilt-loader", m_settings.quiltVersion);
    }

    indexJson.insert("dependencies", dependencies);

    setStatus(tr("Copying files to modpack..."));

    QTemporaryDir tmp;
    if (tmp.isValid()) {
        Json::write(indexJson, tmp.path() + "/modrinth.index.json");

        if (!failedFiles.isEmpty()) {
            QDir tmpDir(tmp.path());
            QDir gameDir(m_instance->gameRoot());
            for (const auto &file : failedFiles) {
                QString src = file.absoluteFilePath();
                tmpDir.mkpath("overrides/" + gameDir.relativeFilePath(file.absolutePath()));
                QString dest = tmpDir.path() + "/overrides/" + gameDir.relativeFilePath(src);
                if (!QFile::copy(file.absoluteFilePath(), dest)) {
                    emitFailed(tr("Failed to copy file %1 to overrides").arg(src));
                    return;
                }
            }

            if (m_settings.includeGameConfig) {
                tmpDir.mkdir("overrides");
                QFile::copy(gameDir.absoluteFilePath("options.txt"), tmpDir.absoluteFilePath("overrides/options.txt"));
            }

            if (m_settings.includeModConfigs) {
                tmpDir.mkdir("overrides");
                FS::copy copy(m_instance->gameRoot() + "/config", tmpDir.absoluteFilePath("overrides/config"));
                copy();
            }
        }

        setStatus(tr("Zipping modpack..."));
        if (!JlCompress::compressDir(m_settings.exportPath, tmp.path())) {
            emitFailed(tr("Failed to create zip file"));
            return;
        }
    } else {
        emitFailed(tr("Failed to create temporary directory"));
        return;
    }

    qDebug() << "Successfully exported Modrinth pack to " << m_settings.exportPath;
    emitSucceeded();
}

void InstanceExportTask::lookupFailed(const QString &reason)
{
    emitFailed(reason);
}

void InstanceExportTask::lookupProgress(qint64 current, qint64 total)
{
    setProgress(current, total);
}

}