#pragma once
#include <QTemporaryDir>

#include "tasks/Task.h"
#include "net/NetJob.h"
#include "Checker.h"
#include "Install.h"
#include "QObjectPtr.h"

namespace UpdaterV2
{

/**
 * Plans an update from what is installed (planInstall) and downloads the files it needs into
 * a new directory. Each file falls back to the files site's other mirrors when one fails or
 * serves something that isn't the size and SHA-256 the manifest lists.
 */
class DownloadTask : public Task
{
    Q_OBJECT
public:
    /**
     * `site` is the files site's first base URL, `root` the installation. `staging` is a
     * template for the new directory: XXXXXX at the end is replaced by something unique. It
     * must be on the same file system as `root`, so installing only renames.
     */
    DownloadTask(shared_qobject_ptr<QNetworkAccessManager> network, const CheckResult &update, const QString &site,
                 const QString &root, const QString &staging, QObject *parent = nullptr);

    // the directory with the downloaded files
    QString stagingDir();
    // what the Installer has to change, once the task succeeded
    QList<Change> changes();

protected:
    void executeTask() override;

private:
    shared_qobject_ptr<QNetworkAccessManager> m_network;
    CheckResult m_update;
    QString m_site;
    QString m_root;
    QTemporaryDir m_stagingDir;
    QList<Change> m_changes;
    NetJob::Ptr m_job;
};

}
