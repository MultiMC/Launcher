/*
 * Copyright 2023 arthomnix
 *
 * This source is subject to the Microsoft Public License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include "tasks/Task.h"
#include "BaseInstance.h"
#include "net/NetJob.h"
#include "ui/dialogs/ModrinthExportDialog.h"

struct ModrinthExportSettings {
    QString version;
    QString name;
    QString description;

    bool includeGameConfig;
    bool includeModConfigs;
    bool includeResourcePacks;
    bool includeShaderPacks;

    QString gameVersion;
    QString forgeVersion;
    QString fabricVersion;
    QString quiltVersion;

    QString exportPath;
};

struct ModrinthLookupData {
    QFileInfo fileInfo;
    QByteArray response;
};

// Using the existing Modrinth::File struct from the importer doesn't actually make much sense here (doesn't support multiple hashes, hash is a byte array rather than a string, no file size, etc)
struct ModrinthFile
{
    QString path;
    QString sha512;
    QString sha1;
    QString download;
    qint64 fileSize;
};

class ModrinthInstanceExportTask : public Task
{
Q_OBJECT

public:
    explicit ModrinthInstanceExportTask(InstancePtr instance, ModrinthExportSettings settings);

protected:
    //! Entry point for tasks.
    virtual void executeTask() override;

private slots:
    void lookupSucceeded();
    void lookupFailed(const QString &);
    void lookupProgress(qint64 current, qint64 total);

private:
    InstancePtr m_instance;
    ModrinthExportSettings m_settings;
    QList<ModrinthLookupData> m_responses;
    NetJob::Ptr m_netJob;
};