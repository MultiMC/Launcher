/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "InstanceImportTask.h"
#include "BaseInstance.h"
#include "FileSystem.h"
#include "Application.h"
#include "MMCZip.h"
#include "NullInstance.h"
#include "settings/INISettingsObject.h"
#include "icons/IconUtils.h"
#include <QtConcurrentRun>

// FIXME: this does not belong here, it's Minecraft/Flame specific
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "Json.h"
#include <quazipdir.h>
#include "modplatform/modrinth/ModrinthPackManifest.h"
#include "modplatform/technic/TechnicPackProcessor.h"

#include "icons/IconList.h"
#include "Application.h"
#include "net/ChecksumValidator.h"

#include <algorithm>
#include <iterator>

InstanceImportTask::InstanceImportTask(const QUrl sourceUrl)
{
    m_sourceUrl = sourceUrl;
}

void InstanceImportTask::executeTask()
{
    if (m_sourceUrl.isLocalFile())
    {
        m_archivePath = m_sourceUrl.toLocalFile();
        processZipPack();
    }
    else
    {
        setStatus(tr("Downloading modpack:\n%1").arg(m_sourceUrl.toString()));
        m_downloadRequired = true;

        const QString path = m_sourceUrl.host() + '/' + m_sourceUrl.path();
        auto entry = APPLICATION->metacache()->resolveEntry("general", path);
        entry->setStale(true);
        m_filesNetJob = new NetJob(tr("Modpack download"), APPLICATION->network());
        m_filesNetJob->addNetAction(Net::Download::makeCached(m_sourceUrl, entry));
        m_archivePath = entry->getFullPath();
        auto job = m_filesNetJob.get();
        connect(job, &NetJob::succeeded, this, &InstanceImportTask::downloadSucceeded);
        connect(job, &NetJob::progress, this, &InstanceImportTask::downloadProgressChanged);
        connect(job, &NetJob::failed, this, &InstanceImportTask::downloadFailed);
        m_filesNetJob->start();
    }
}

void InstanceImportTask::downloadSucceeded()
{
    processZipPack();
    m_filesNetJob.reset();
}

void InstanceImportTask::downloadFailed(QString reason)
{
    emitFailed(reason);
    m_filesNetJob.reset();
}

void InstanceImportTask::downloadProgressChanged(qint64 current, qint64 total)
{
    setProgress(current / 2, total);
}

void InstanceImportTask::processZipPack()
{
    setStatus(tr("Extracting modpack"));
    QDir extractDir(m_stagingPath);
    qDebug() << "Attempting to create instance from" << m_archivePath;

    // open the zip and find relevant files in it
    m_packZip.reset(new QuaZip(m_archivePath));
    if (!m_packZip->open(QuaZip::mdUnzip))
    {
        emitFailed(tr("Unable to open supplied modpack zip file."));
        return;
    }

    QStringList blacklist = {"instance.cfg", "manifest.json"};
    QString mmcFound = MMCZip::findFolderOfFileInZip(m_packZip.get(), "instance.cfg");
    bool technicFound = QuaZipDir(m_packZip.get()).exists("/bin/modpack.jar") || QuaZipDir(m_packZip.get()).exists("/bin/version.json");
    QString modrinthFound = MMCZip::findFolderOfFileInZip(m_packZip.get(), "modrinth.index.json");
    QString root;
    if(!mmcFound.isNull())
    {
        // process as MultiMC instance/pack
        qDebug() << "MultiMC:" << mmcFound;
        root = mmcFound;
        m_modpackType = ModpackType::MultiMC;
    }
    else if (technicFound)
    {
        // process as Technic pack
        qDebug() << "Technic:" << technicFound;
        extractDir.mkpath(".minecraft");
        extractDir.cd(".minecraft");
        m_modpackType = ModpackType::Technic;
    }
    else if(!modrinthFound.isNull())
    {
        // process as Modrinth pack
        qDebug() << "Modrinth:" << modrinthFound;
        root = modrinthFound;
        m_modpackType = ModpackType::Modrinth;
    }
    if(m_modpackType == ModpackType::Unknown)
    {
        emitFailed(tr("Archive does not contain a recognized modpack type."));
        return;
    }

    // make sure we extract just the pack
    m_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), MMCZip::extractSubDir, m_packZip.get(), root, extractDir.absolutePath());
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, &InstanceImportTask::extractFinished);
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, &InstanceImportTask::extractAborted);
    m_extractFutureWatcher.setFuture(m_extractFuture);
}

void InstanceImportTask::extractFinished()
{
    m_packZip.reset();
    if (!m_extractFuture.result())
    {
        emitFailed(tr("Failed to extract modpack"));
        return;
    }
    QDir extractDir(m_stagingPath);

    qDebug() << "Fixing permissions for extracted pack files...";
    QDirIterator it(extractDir, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        auto filepath = it.next();
        QFileInfo file(filepath);
        auto permissions = QFile::permissions(filepath);
        auto origPermissions = permissions;
        if(file.isDir())
        {
            // Folder +rwx for current user
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser | QFileDevice::Permission::ExeUser;
        }
        else
        {
            // File +rw for current user
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser;
        }
        if(origPermissions != permissions)
        {
            if(!QFile::setPermissions(filepath, permissions))
            {
                logWarning(tr("Could not fix permissions for %1").arg(filepath));
            }
            else
            {
                qDebug() << "Fixed" << filepath;
            }
        }
    }

    switch(m_modpackType)
    {
        case ModpackType::MultiMC:
            processMultiMC();
            return;
        case ModpackType::Technic:
            processTechnic();
            return;
        case ModpackType::Modrinth:
            processModrinth();
            return;
        case ModpackType::Unknown:
            emitFailed(tr("Archive does not contain a recognized modpack type."));
            return;
    }
}

void InstanceImportTask::extractAborted()
{
    emitFailed(tr("Instance import has been aborted."));
    return;
}

void InstanceImportTask::processTechnic()
{
    shared_qobject_ptr<Technic::TechnicPackProcessor> packProcessor = new Technic::TechnicPackProcessor();
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::succeeded, this, &InstanceImportTask::emitSucceeded);
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::failed, this, &InstanceImportTask::emitFailed);
    packProcessor->run(m_globalSettings, m_instName, m_instIcon, m_stagingPath);
}

void InstanceImportTask::processMultiMC()
{
    QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(configPath);
    instanceSettings->registerSetting("InstanceType", "Legacy");

    NullInstance instance(m_globalSettings, instanceSettings, m_stagingPath);

    // reset time played on import... because packs.
    instance.resetTimePlayed();

    // set a new nice name
    instance.setName(m_instName);

    // if the icon was specified by user, use that. otherwise pull icon from the pack
    if (m_instIcon != "default")
    {
        instance.setIconKey(m_instIcon);
    }
    else
    {
        m_instIcon = instance.iconKey();

        auto importIconPath = IconUtils::findBestIconIn(instance.instanceRoot(), m_instIcon);
        if (!importIconPath.isNull() && QFile::exists(importIconPath))
        {
            // import icon
            auto iconList = APPLICATION->icons();
            if (iconList->iconFileExists(m_instIcon))
            {
                iconList->deleteIcon(m_instIcon);
            }
            iconList->installIcons({importIconPath});
        }
    }
    emitSucceeded();
}

namespace {
bool mergeOverrides(const QString &fromDir, const QString &toDir) {
    QDir dir(fromDir);
    if(!dir.exists()) {
        return true;
    }
    if(!FS::ensureFolderPathExists(toDir)) {
        return false;
    }
    const int absSourcePathLength = dir.absoluteFilePath(fromDir).length();

    QDirIterator it(fromDir, QDirIterator::Subdirectories);
    while (it.hasNext()){
        it.next();
        const auto fileInfo = it.fileInfo();
        auto fileName = fileInfo.fileName();
        if(fileName == "." || fileName == "..") {
            continue;
        }
        const QString subPathStructure = fileInfo.absoluteFilePath().mid(absSourcePathLength);
        const QString constructedAbsolutePath = toDir + subPathStructure;

        if(fileInfo.isDir()){
            //Create directory in target folder
            dir.mkpath(constructedAbsolutePath);
        } else if(fileInfo.isFile()) {
            QFileInfo targetFileInfo(constructedAbsolutePath);
            if(targetFileInfo.exists()) {
                continue;
            }
            // move
            QFile::rename(fileInfo.absoluteFilePath(), constructedAbsolutePath);
        }
    }

    dir.removeRecursively();
    return true;
}

}

void InstanceImportTask::processModrinth() {
    std::vector<Modrinth::File> files;
    QString minecraftVersion, fabricVersion, quiltVersion, forgeVersion;
    try
    {
        QString indexPath = FS::PathCombine(m_stagingPath, "modrinth.index.json");
        auto doc = Json::requireDocument(indexPath);
        auto obj = Json::requireObject(doc, "modrinth.index.json");
        int formatVersion = Json::requireInteger(obj, "formatVersion", "modrinth.index.json");
        if (formatVersion == 1)
        {
            auto game = Json::requireString(obj, "game", "modrinth.index.json");
            if (game != "minecraft")
            {
                throw JSONValidationError("Unknown game: " + game);
            }

            auto jsonFiles = Json::requireIsArrayOf<QJsonObject>(obj, "files", "modrinth.index.json");
            for(auto & obj: jsonFiles) {
                Modrinth::File file;
                auto dirtyPath = Json::requireString(obj, "path");
                dirtyPath.replace('\\', '/');
                auto simplifiedPath = QDir::cleanPath(dirtyPath);
                QFileInfo fileInfo (simplifiedPath);
                if(simplifiedPath.startsWith("../") || simplifiedPath.contains("/../") || fileInfo.isAbsolute()) {
                    throw JSONValidationError("Invalid path found in modpack files:\n\n" + simplifiedPath);
                }
                file.path = simplifiedPath;

                // env doesn't have to be present, in that case mod is required
                auto env = Json::ensureObject(obj, "env");
                auto clientEnv = Json::ensureString(env, "client", "required");

                if(clientEnv == "required") {
                    // NOOP
                }
                else if(clientEnv == "optional") {
                    file.path += ".disabled";
                }
                else if(clientEnv == "unsupported") {
                    continue;
                }

                QJsonObject hashes = Json::requireObject(obj, "hashes");
                QString hash;
                QCryptographicHash::Algorithm hashAlgorithm;
                hash = Json::ensureString(hashes, "sha256");
                hashAlgorithm = QCryptographicHash::Sha256;
                if (hash.isEmpty())
                {
                    hash = Json::ensureString(hashes, "sha512");
                    hashAlgorithm = QCryptographicHash::Sha512;
                    if (hash.isEmpty())
                    {
                        hash = Json::ensureString(hashes, "sha1");
                        hashAlgorithm = QCryptographicHash::Sha1;
                        if (hash.isEmpty())
                        {
                            throw JSONValidationError("No hash found for: " + file.path);
                        }
                    }
                }
                file.hash = QByteArray::fromHex(hash.toLatin1());
                file.hashAlgorithm = hashAlgorithm;
                // Do not use requireUrl, which uses StrictMode, instead use QUrl's default TolerantMode (as Modrinth seems to incorrectly handle spaces)
                file.download = Json::requireValueString(Json::ensureArray(obj, "downloads").first(), "Download URL for " + file.path);
                if (!file.download.isValid())
                {
                    throw JSONValidationError("Download URL for " + file.path + " is not a correctly formatted URL");
                }
                files.push_back(file);
            }

            auto dependencies = Json::requireObject(obj, "dependencies", "modrinth.index.json");
            for (auto it = dependencies.begin(), end = dependencies.end(); it != end; ++it)
            {
                QString name = it.key();
                if (name == "minecraft")
                {
                    if (!minecraftVersion.isEmpty())
                        throw JSONValidationError("Duplicate Minecraft version");
                    minecraftVersion = Json::requireValueString(*it, "Minecraft version");
                }
                else if (name == "fabric-loader")
                {
                    if (!fabricVersion.isEmpty())
                        throw JSONValidationError("Duplicate Fabric Loader version");
                    fabricVersion = Json::requireValueString(*it, "Fabric Loader version");
                }
                else if (name == "quilt-loader")
                {
                    if (!quiltVersion.isEmpty())
                        throw JSONValidationError("Duplicate Quilt Loader version");
                    quiltVersion = Json::requireValueString(*it, "Quilt Loader version");
                }
                else if (name == "forge")
                {
                    if (!forgeVersion.isEmpty())
                        throw JSONValidationError("Duplicate Forge version");
                    forgeVersion = Json::requireValueString(*it, "Forge version");
                }
                else
                {
                    throw JSONValidationError("Unknown dependency type: " + name);
                }
            }
        }
        else
        {
            throw JSONValidationError(QStringLiteral("Unknown format version: %s").arg(formatVersion));
        }
        QFile::remove(indexPath);
    }
    catch (const JSONValidationError &e)
    {
        emitFailed(tr("Could not understand pack index:\n") + e.cause());
        return;
    }
    QString clientOverridePath = FS::PathCombine(m_stagingPath, "client-overrides");
    if (QFile::exists(clientOverridePath)) {
        QString mcPath = FS::PathCombine(m_stagingPath, ".minecraft");
        if (!QFile::rename(clientOverridePath, mcPath)) {
            emitFailed(tr("Could not rename the overrides folder:\n") + "overrides");
            return;
        }
    }

    // TODO: only extract things we actually want instead of everything only to just delete it afterwards ...
    if(!mergeOverrides(FS::PathCombine(m_stagingPath, "client-overrides"), FS::PathCombine(m_stagingPath, ".minecraft"))) {
        emitFailed(tr("Could not merge the overrides folder:\n") + "client-overrides");
        return;
    }

    if(!mergeOverrides(FS::PathCombine(m_stagingPath, "overrides"), FS::PathCombine(m_stagingPath, ".minecraft"))) {
        emitFailed(tr("Could not merge the overrides folder:\n") + "overrides");
        return;
    }

    FS::deletePath(FS::PathCombine(m_stagingPath, "server-overrides"));


    QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(configPath);
    instanceSettings->registerSetting("InstanceType", "Legacy");
    instanceSettings->set("InstanceType", "OneSix");
    MinecraftInstance instance(m_globalSettings, instanceSettings, m_stagingPath);
    auto components = instance.getPackProfile();
    components->buildingFromScratch();
    components->setComponentVersion("net.minecraft", minecraftVersion, true);
    if (!fabricVersion.isEmpty())
        components->setComponentVersion("net.fabricmc.fabric-loader", fabricVersion, true);
    if (!quiltVersion.isEmpty())
        components->setComponentVersion("org.quiltmc.quilt-loader", quiltVersion, true);
    if (!forgeVersion.isEmpty())
        components->setComponentVersion("net.minecraftforge", forgeVersion, true);
    if (m_instIcon != "default")
    {
        instance.setIconKey(m_instIcon);
    }
    instance.setName(m_instName);
    instance.saveNow();

    m_filesNetJob = new NetJob(tr("Mod download"), APPLICATION->network());
    for (auto &file : files)
    {
        auto path = FS::PathCombine(m_stagingPath, ".minecraft", file.path);
        qDebug() << "Will download" << file.download << "to" << path;
        auto dl = Net::Download::makeFile(file.download, path);
        dl->addValidator(new Net::ChecksumValidator(file.hashAlgorithm, file.hash));
        m_filesNetJob->addNetAction(dl);
    }
    connect(m_filesNetJob.get(), &NetJob::succeeded, this, [&]()
    {
        m_filesNetJob.reset();
        emitSucceeded();
    });
    connect(m_filesNetJob.get(), &NetJob::failed, [&](const QString &reason)
    {
        m_filesNetJob.reset();
        emitFailed(reason);
    });
    connect(m_filesNetJob.get(), &NetJob::progress, [&](qint64 current, qint64 total)
    {
        setProgress(current, total);
    });
    setStatus(tr("Downloading mods..."));
    m_filesNetJob->start();
}
