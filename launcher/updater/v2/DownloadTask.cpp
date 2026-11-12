#include "DownloadTask.h"

#include <QDebug>
#include <QFileInfo>

#include "net/ChecksumValidator.h"
#include "net/Download.h"
#include "net/SizeValidator.h"

namespace UpdaterV2
{

namespace {
// a transfer that gets nothing for this long fails, and the next mirror is tried
const int StallTimeout = 30 * 1000;
}

DownloadTask::DownloadTask(shared_qobject_ptr<QNetworkAccessManager> network, const CheckResult &update, const QString &site,
                           const QString &root, const QString &staging, QObject *parent)
    : Task(parent), m_network(network), m_update(update), m_site(site), m_root(root), m_stagingDir(staging)
{
    m_stagingDir.setAutoRemove(false);
}

QString DownloadTask::stagingDir()
{
    return m_stagingDir.path();
}

QList<Change> DownloadTask::changes()
{
    return m_changes;
}

void DownloadTask::executeTask()
{
    if(!m_stagingDir.isValid())
    {
        emitFailed(tr("Couldn't create a folder for the update's files."));
        return;
    }
    removeOldStaging(QFileInfo(m_stagingDir.path()).absolutePath(), m_stagingDir.path());
    setStatus(tr("Comparing the installed files with build %1...").arg(m_update.build));
    QList<Download> downloads;
    QList<Change> changes;
    planInstall(m_update.variant, m_update.haveCurrent ? &m_update.current : nullptr, m_root, m_stagingDir.path(), downloads, changes);
    qint64 totalBytes = 0;
    for(auto &download: downloads)
    {
        totalBytes += download.file.size;
    }
    qDebug() << "Updater:" << changes.size() << "changes," << downloads.size() << "files to download," << totalBytes << "bytes, to" << m_stagingDir.path();
    if(downloads.isEmpty())
    {
        m_changes = changes;
        emitSucceeded();
        return;
    }

    setStatus(tr("Downloading build %1...").arg(m_update.build));
    m_job = new NetJob(tr("Update to build %1").arg(m_update.build), m_network);
    for(auto &download: downloads)
    {
        auto dl = Net::Download::makeFile(treeUrl(m_site, contentPath(download.file.sha256)), download.target);
        dl->addValidator(new Net::SizeValidator(download.file.size));
        dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha256, QByteArray::fromHex(download.file.sha256.toLatin1())));
        dl->setStallTimeout(StallTimeout);
        m_job->addNetAction(dl);
    }
    connect(m_job.get(), &NetJob::progress, this, &Task::setProgress);
    connect(m_job.get(), &NetJob::succeeded, this, [this, changes]()
    {
        m_job.reset();
        m_changes = changes;
        emitSucceeded();
    });
    connect(m_job.get(), &NetJob::failed, this, [this](QString reason)
    {
        m_job.reset();
        emitFailed(tr("Failed to download the update:\n%1").arg(reason));
    });
    m_job->start();
}

}
