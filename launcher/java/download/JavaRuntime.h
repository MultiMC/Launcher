/* Copyright 2025-2026 Mehmet Samet Duman
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace JavaDownload {

struct RuntimeVersion {
    int major = 0;
    int minor = 0;
    int security = 0;
    int build = 0;

    QString toString() const {
        if (build > 0)
            return QString("%1.%2.%3+%4").arg(major).arg(minor).arg(security).arg(build);
        return QString("%1.%2.%3").arg(major).arg(minor).arg(security);
    }
};

struct RuntimeEntry {
    QString name;
    QString url;
    QString checksumHash;
    QString checksumType;
    QString downloadType;
    QString packageType;
    QString releaseTime;
    QString runtimeOS;
    QString vendor;
    RuntimeVersion version;
};

struct JavaVersionInfo {
    QString uid;
    QString versionId;
    QString name;
    QString sha256;
    QString releaseTime;
    bool recommended = false;
};

struct JavaProviderInfo {
    QString uid;
    QString name;
    QString iconName;

    static QList<JavaProviderInfo> availableProviders() {
        return {
            { "net.adoptium.java", "Eclipse Adoptium", "adoptium" },
            { "com.azul.java", "Azul Zulu", "azul" },
            { "com.ibm.java", "IBM Semeru (OpenJ9)", "openj9_hex_custom" },
            { "net.minecraft.java", "Mojang", "mojang" },
        };
    }
};

inline QString currentRuntimeOS() {
#if defined(Q_OS_LINUX)
    #if defined(__aarch64__)
        return "linux-arm64";
    #elif defined(__riscv)
        return "linux-riscv64";
    #else
        return "linux-x64";
    #endif
#elif defined(Q_OS_MACOS)
    #if defined(__aarch64__)
        return "mac-os-arm64";
    #else
        return "mac-os-x64";
    #endif
#elif defined(Q_OS_WIN)
    #if defined(__aarch64__) || defined(_M_ARM64)
        return "windows-arm64";
    #else
        return "windows-x64";
    #endif
#else
    return "unknown";
#endif
}

inline RuntimeEntry parseRuntimeEntry(const QJsonObject &obj) {
    RuntimeEntry entry;
    entry.name = obj["name"].toString();
    entry.url = obj["url"].toString();
    entry.downloadType = obj["downloadType"].toString();
    entry.packageType = obj["packageType"].toString();
    entry.releaseTime = obj["releaseTime"].toString();
    entry.runtimeOS = obj["runtimeOS"].toString();
    entry.vendor = obj["vendor"].toString();

    auto checksum = obj["checksum"].toObject();
    entry.checksumHash = checksum["hash"].toString();
    entry.checksumType = checksum["type"].toString();

    auto ver = obj["version"].toObject();
    entry.version.major = ver["major"].toInt();
    entry.version.minor = ver["minor"].toInt();
    entry.version.security = ver["security"].toInt();
    entry.version.build = ver["build"].toInt();

    return entry;
}

inline QList<RuntimeEntry> parseRuntimes(const QJsonObject &obj) {
    QList<RuntimeEntry> entries;
    auto arr = obj["runtimes"].toArray();
    for (const auto &val : arr) {
        entries.append(parseRuntimeEntry(val.toObject()));
    }
    return entries;
}

inline QList<JavaVersionInfo> parseVersionIndex(const QJsonObject &obj, const QString &uid) {
    QList<JavaVersionInfo> versions;
    auto arr = obj["versions"].toArray();
    for (const auto &val : arr) {
        auto vObj = val.toObject();
        JavaVersionInfo info;
        info.uid = uid;
        info.versionId = vObj["version"].toString();
        info.sha256 = vObj["sha256"].toString();
        info.releaseTime = vObj["releaseTime"].toString();
        info.recommended = vObj["recommended"].toBool(false);
        info.name = obj["name"].toString();
        versions.append(info);
    }
    return versions;
}

} // namespace JavaDownload
