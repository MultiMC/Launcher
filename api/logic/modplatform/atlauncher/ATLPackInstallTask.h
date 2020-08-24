#pragma once

#include "ATLPackManifest.h"

#include "InstanceTask.h"
#include "multimc_logic_export.h"
#include "net/NetJob.h"

namespace ATLauncher {

class MULTIMC_LOGIC_EXPORT PackInstallTask : public InstanceTask
{
Q_OBJECT

public:
    explicit PackInstallTask(QString pack, QString version);
    virtual ~PackInstallTask(){}

    bool abort() override;

protected:
    virtual void executeTask() override;

private slots:
    void onDownloadSucceeded();
    void onDownloadFailed(QString reason);

private:
    void install();

private:
    NetJobPtr jobPtr;
    QByteArray response;

    QString m_pack;
    QString m_version_name;
    Version m_version;

};

}
