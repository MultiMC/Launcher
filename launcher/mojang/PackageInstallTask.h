/*
 * Copyright 2023 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include "tasks/Task.h"
#include "net/Mode.h"

#include <memory>
#include <net/NetJob.h>

struct PackageInstallTaskData;

class PackageInstallTask : public Task
{
    Q_OBJECT
public:
    enum class Mode
    {
        Launch,
        Resolution
    };

public:
    explicit PackageInstallTask(
        shared_qobject_ptr<QNetworkAccessManager> network,
        Net::Mode netmode,
        QString version,
        QString packageURL,
        QString targetPath,
        QObject *parent = 0
    );
    virtual ~PackageInstallTask();

protected:
    void executeTask() override;

private slots:
    void inspectionFinished();
    void manifestFetchFinished();

    void processInputs();

    void downloadsSucceeded();
    void downloadsFailed(QString reason);

private:
    std::unique_ptr<PackageInstallTaskData> d;
};


