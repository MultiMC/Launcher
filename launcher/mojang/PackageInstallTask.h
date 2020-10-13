#pragma once

#include "tasks/Task.h"
#include "net/Mode.h"
#include "QObjectPtr.h"

#include <memory>
#include <QNetworkAccessManager>

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


