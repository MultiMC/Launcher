/*
 * Copyright 2022 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "Model.h"
#include "Application.h"

#include <MMCStrings.h>
#include <Version.h>

#include <QtMath>
#include <QLabel>

#include <RWStorage.h>

#include <BuildConfig.h>
#include <FileSystem.h>
#include <Json.h>

#if defined(Q_OS_WIN32)
#include <windows.h>
constexpr int BUFFER_SIZE = 1024;
#endif

namespace ImportFTB {

Model::Model(QObject *parent) : QAbstractListModel(parent)
{
}

int Model::rowCount(const QModelIndex &parent) const
{
    return modpacks.size();
}

int Model::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant Model::data(const QModelIndex &index, int role) const
{
    int pos = index.row();
    if(pos >= modpacks.size() || pos < 0 || !index.isValid())
    {
        return QVariant();
    }

    Modpack pack = modpacks.at(pos);
    if(role == Qt::DisplayRole)
    {
        return pack.name;
    }
    else if(role == Qt::DecorationRole)
    {
        return pack.icon;
    }
    else if(role == Qt::UserRole)
    {
        QVariant v;
        v.setValue(pack);
        return v;
    }

    return QVariant();
}

namespace {

#if defined (Q_OS_OSX)
QString getFTBASettingsPath() {
    return FS::PathCombine(QDir::homePath(), "Library/Application Support/.ftba/bin/settings.json");
}
#elif defined(Q_OS_WIN32)
QString getFTBASettingsPath() {
    wchar_t buf[BUFFER_SIZE];
    if(!GetEnvironmentVariableW(L"LOCALAPPDATA", buf, BUFFER_SIZE))
    {
        return QString();
    }
    QString appDataLocal = QString::fromWCharArray(buf);
    QString settingsPath = FS::PathCombine(appDataLocal, ".ftba/bin/settings.json");
    return settingsPath;
}
#else
QString getFTBASettingsPath() {
    return FS::PathCombine(QDir::homePath(), ".ftba/bin/settings.json");
}
#endif

QString getFTBAInstances() {
    QByteArray data;
    auto settingsPath = getFTBASettingsPath();
    try
    {
        data = FS::read(settingsPath);
    }
    catch (const Exception &e)
    {
        qWarning() << "Could not read FTB App settings file: " << settingsPath;
        return QString();
    }

    try
    {
        auto document = Json::requireDocument(data);
        auto object = Json::requireObject(document);
        return Json::requireString(object, "instanceLocation");
    }
    catch (Json::JsonException & e)
    {
        qCritical() << "Could not read FTB App settings file as JSON: " << e.cause();
        return QString();
    }
}

/*
Reference from an FTB App file, as of 28.05.2022
{
  "_private": false,
  "uuid": "25850fc6-ec61-4bc6-8397-77e4497bf922",
  "id": 95,
  "art": "data:image/png;base64,...==",
  "path": "/home/peterix/.ftba/instances/25850fc6-ec61-4bc6-8397-77e4497bf922",
  "versionId": 2127,
  "name": "FTB Presents Direwolf20 1.18",
  "minMemory": 4096,
  "recMemory": 6144,
  "memory": 6144,
  "version": "1.2.0",
  "dir": "/home/peterix/.ftba/instances/25850fc6-ec61-4bc6-8397-77e4497bf922",
  "authors": [
    "FTB Team"
  ],
  "description": "Direwolf's modded legacy started all the way back in 2011. With 5 modpacks from the FTB Team, dedicated to creating a simple yet unique form of kitchen-sink modpacks.\n\nThis new iteration of Direwolf's modpack is packed with hand-selected mods from Direwolf20 himself, the FTB Team have worked hard in forging a kitchen-sink styled modded Minecraft experience that just about anyone can play. \n\nFollow along with Direwolf on his YouTube series, learning with him as you venture out into new modded territory. Improve his designs as you build up more infrastructure and automate anything and everything. The world is yours, for you to do anything in.",
  "mcVersion": "1.18.2",
  "jvmArgs": "",
  "embeddedJre": true,
  "url": "",
  "artUrl": "https://apps.modpacks.ch/modpacks/art/90/1024_1024.png",
  "width": 1920,
  "height": 1080,
  "modLoader": "1.18.2-forge-40.1.20",
  "isModified": false,
  "isImport": false,
  "cloudSaves": false,
  "hasInstMods": false,
  "installComplete": true,
  "packType": 0,
  "totalPlayTime": 0,
  "lastPlayed": 1653168242
}
*/

bool parseModpackJson(const QByteArray& data, Modpack & out) {
    try
    {
        auto document = Json::requireDocument(data);
        auto object = Json::requireObject(document);
        bool isInstalled = Json::requireBoolean(object, "installComplete");
        if(!isInstalled) {
            return false;
        }
        out.id = Json::requireInteger(object, "id");
        out.name = Json::requireString(object, "name");
        out.version = Json::requireString(object, "version");
        out.description = Json::requireString(object, "description");
        auto authorsArray = Json::requireArray(object, "authors");
        for(auto author: authorsArray) {
            out.authors.append(Json::requireString(author));
        }

        out.mcVersion = Json::requireString(object, "mcVersion");
        out.modLoader = Json::requireString(object, "modLoader");
        out.hasInstMods = Json::requireBoolean(object, "hasInstMods");

        out.minMemory = Json::requireInteger(object, "minMemory");
        out.recMemory = Json::requireInteger(object, "recMemory");
        out.memory = Json::requireInteger(object, "memory");
        return true;
    }
    catch (Json::JsonException & e)
    {
        qCritical() << "Could not read FTB App settings file as JSON: " << e.cause();
        return false;
    }
}

bool tryLoadInstance(const QString& location, Modpack & out) {
    QFileInfo dirInfo(location);

    if(dirInfo.isSymLink())
        return false;

    QByteArray data;
    auto jsonLocation = FS::PathCombine(location, "instance.json");
    try
    {
        data = FS::read(jsonLocation);
        if(!parseModpackJson(data, out)) {
            return false;
        }
        out.instanceDir = location;
        out.iconPath = FS::PathCombine(location, "folder.jpg");
        out.icon = QIcon(out.iconPath);
        return true;
    }
    catch (const Exception &e)
    {
        qWarning() << "Could not read FTB App instance JSON file: " << jsonLocation;
        return false;
    }
    return false;
}

}

void Model::reload() {
    beginResetModel();
    modpacks.clear();
    QString instancesLocation = getFTBAInstances();
    if(instancesLocation.isEmpty()) {
        qDebug() << "FTB instances are not found...";
    }
    else {
        qDebug() << "Discovering FTB instances in" << instancesLocation;
        QDirIterator iter(instancesLocation, QDir::Dirs | QDir::NoDot | QDir::NoDotDot | QDir::Readable | QDir::Hidden, QDirIterator::FollowSymlinks);
        while (iter.hasNext()) {
            QString subDir = iter.next();
            ImportFTB::Modpack out;
            if(!tryLoadInstance(subDir, out)) {
                continue;
            }
            modpacks.append(out);
        }
    }
    endResetModel();
}

}
