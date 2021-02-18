#pragma once
#include "InstanceTask.h"
#include "net/NetJob.h"
#include "quazip.h"
#include "quazipdir.h"
#include "meta/Index.h"
#include "meta/Version.h"
#include "meta/VersionList.h"
#include "PackHelpers.h"

#include <nonstd/optional>

namespace LegacyFTB {

class MULTIMC_LOGIC_EXPORT PackInstallTask : public InstanceTask
{
    Q_OBJECT

public:
    explicit PackInstallTask(Modpack pack, QString version);
    virtual ~PackInstallTask(){}

    bool abort() override;

protected:
    //! Entry point for tasks.
    virtual void executeTask() override;

private:
    void downloadPack();
    void unzip();
    void install();

private slots:
    void onDownloadSucceeded();
    void onDownloadFailed(QString reason);
    void onDownloadProgress(qint64 current, qint64 total);

    void onUnzipFinished();
    void onUnzipCanceled();

private: /* data */
    bool abortable = false;
    std::unique_ptr<QuaZip> m_packZip;
    QFuture<nonstd::optional<QStringList>> m_extractFuture;
    QFutureWatcher<nonstd::optional<QStringList>> m_extractFutureWatcher;
    NetJobPtr netJobContainer;
    QString archivePath;

    Modpack m_pack;
    QString m_version;
};

}
