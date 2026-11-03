// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QStringList>
#include <QCryptographicHash>
#include <QUrl>
#include <functional>

namespace Modrinth {
namespace Update {
struct Error { QString message; };
struct File {
    QString path, hash, identity;
    QString algorithm = "sha512";
    QUrl url;
    qint64 size = 0;
    bool optional = false;
    bool overrideFile = false;
};
struct Pack {
    QJsonObject manifest;
    QMap<QString, File> files;
};
struct Plan {
    QMap<QString, File> writes;
    QStringList removes, skipped, added, changed;
    QJsonObject metadata;
    // Hashes of all files read while planning, including preserved user content.
    QMap<QString, QString> observed;
    qint64 downloadBytes = 0;
};
using Progress = std::function<void(const QString &)>;
QString safePath(const QString &root, const QString &relative);
QString digest(const QString &path, const QString &algorithm = "sha512");
QJsonObject readJson(const QString &path);
void writeJson(const QString &path, const QJsonObject &object);
void validateArchive(const QString &archive, const QString &destination);
Pack readPack(const QString &archive, const QString &overrides);
QJsonObject baseline(const Pack &pack, const QString &gameRoot, const QString &project, const QString &version);
Plan plan(const QJsonObject &baseline, const Pack &target, const QString &gameRoot, const QString &project, const QString &version);
QJsonObject latestRelease(const QJsonArray &versions, const QString &minecraft, const QString &loader, const QJsonObject &current);
void validateCompatibility(const Pack &pack, const QString &minecraft, const QString &loader);
QString loaderKey(const QJsonObject &dependencies);
QString componentUid(const QString &loader);
QJsonObject updateLoaderComponent(QJsonObject components, const QString &loader, const QString &version);
QStringList modIds(const QString &jar);
void validateMods(const QString &gameRoot, const QString &staged, const Plan &plan);
void verifyUnchanged(const QString &gameRoot, const Plan &plan);
// Staged paths are relative to the instance root, not the game root.
void commit(const QString &instanceRoot, const QString &staged, const QStringList &writes,
            const QStringList &removes, const Progress &progress = Progress(),
            const std::function<qint64()> &spaceAvailable = std::function<qint64()>());
void restore(const QString &instanceRoot, const Progress &progress = Progress());
bool pending(const QString &instanceRoot);
bool hasBackup(const QString &instanceRoot);
}
}
