#include "PackageInstallTask.h"

#include <QThread>
#include <QFuture>
#include <QFutureWatcher>
#include <QRunnable>
#include <QtConcurrent>

#include <net/NetJob.h>
#include <net/ChecksumValidator.h>
#include <Env.h>

#include "PackageManifest.h"
#include <FileSystem.h>

using Package = mojang_files::Package;
using UpdateOperations = mojang_files::UpdateOperations;

namespace {
struct PackageInstallTaskData
{
    QString root;
    QString version;
    QString packageURL;
    Net::Mode netmode;
    QFuture<Package> inspectionFuture;
    QFutureWatcher<Package> inspectionWatcher;
    Package inspectedPackage;

    NetJobPtr manifestDownloadJob;
    bool manifestDone = false;
    Package downloadedPackage;

    UpdateOperations updateOps;
    NetJobPtr mainDownloadJob;
};

class InspectFolder
{
public:
    InspectFolder(const QString & folder) : folder(folder) {}
    Package operator()()
    {
        return Package::fromInspectedFolder(folder);
    }
private:
    QString folder;
};

class ParsingValidator : public Net::Validator
{
public: /* con/des */
    ParsingValidator(Package &package) : m_package(package)
    {
    }
    virtual ~ParsingValidator() = default;

public: /* methods */
    bool init(QNetworkRequest &) override
    {
        m_package.valid = false;
        return true;
    }
    bool write(QByteArray & data) override
    {
        this->data.append(data);
        return true;
    }
    bool abort() override
    {
        return true;
    }
    bool validate(QNetworkReply &) override
    {
        m_package = Package::fromManifestContents(data);
        return m_package.valid;
    }

private: /* data */
    QByteArray data;
    Package &m_package;
};

}

PackageInstallTask::PackageInstallTask(
    Net::Mode netmode,
    QString version,
    QString packageURL,
    QString targetPath,
    QObject* parent
) : Task(parent) {
    d.reset(new PackageInstallTaskData);
    d->netmode = netmode;
    d->root = targetPath;
    d->packageURL = packageURL;
    d->version = version;
}

PackageInstallTask::~PackageInstallTask() {}

void PackageInstallTask::executeTask() {
    // inspect the data folder in a thread
    d->inspectionFuture = QtConcurrent::run(QThreadPool::globalInstance(), InspectFolder(FS::PathCombine(d->root, "data")));
    connect(&d->inspectionWatcher, &QFutureWatcher<Package>::finished, this, &PackageInstallTask::inspectionFinished);
    d->inspectionWatcher.setFuture(d->inspectionFuture);

    // while inspecting, grab the manifest from remote
    d->manifestDownloadJob.reset(new NetJob(QObject::tr("Download of package manifest %1").arg(d->packageURL)));
    auto url = d->packageURL;
    auto dl = Net::Download::makeFile(url, FS::PathCombine(d->root, "manifest.json"));
    /*
     * The validator parses the file and loads it into the object.
     * If that fails, the file is not written to storage.
     */
    dl->addValidator(new ParsingValidator(d->downloadedPackage));
    d->manifestDownloadJob->addNetAction(dl);
    auto job = d->manifestDownloadJob.get();
    connect(job, &NetJob::finished, this, &PackageInstallTask::manifestFetchFinished);
    d->manifestDownloadJob->start();
}

void PackageInstallTask::inspectionFinished() {
    d->inspectedPackage = d->inspectionWatcher.result();
    processInputs();
}

void PackageInstallTask::manifestFetchFinished()
{
    d->manifestDone = true;
    d->manifestDownloadJob.reset();
    processInputs();
}

void PackageInstallTask::processInputs()
{
    if(!d->manifestDone) {
        return;
    }
    if(!d->inspectionFuture.isFinished()) {
        return;
    }

    if(!d->downloadedPackage.valid) {
        emitFailed("Downloading package manifest failed...");
        return;
    }

    if(!d->inspectedPackage.valid) {
        emitFailed("Inspecting local data folder failed...");
        return;
    }

    d->updateOps = UpdateOperations::resolve(d->inspectedPackage, d->downloadedPackage);

    if(!d->updateOps.valid) {
        emitFailed("Unable to determine update actions...");
        return;
    }

    if(d->updateOps.empty()) {
        emitSucceeded();
    }

    auto dataRoot = FS::PathCombine(d->root, "data");

    // first, ensure data path exists
    QDir temp;
    temp.mkpath(dataRoot);

    for(auto & rm: d->updateOps.deletes) {
        auto filePath = FS::PathCombine(dataRoot, rm.toString());
        qDebug() << "RM" << filePath;
        QFile::remove(filePath);
    }

    for(auto & rmdir: d->updateOps.rmdirs) {
        auto folderPath = FS::PathCombine(dataRoot, rmdir.toString());
        qDebug() << "RMDIR" << folderPath;
        QDir dir;
        dir.rmdir(folderPath);
    }

    for(auto & mkdir: d->updateOps.mkdirs) {
        auto folderPath = FS::PathCombine(dataRoot, mkdir.toString());
        qDebug() << "MKDIR" << folderPath;
        QDir dir;
        dir.mkdir(folderPath);
    }

    for(auto & mklink: d->updateOps.mklinks) {
        auto linkPath = FS::PathCombine(dataRoot, mklink.first.toString());
        auto linkTarget = mklink.second.toString();
        qDebug() << "MKLINK" << linkPath << "->" << linkTarget;
        QFile::link(linkTarget, linkPath);
    }

    for(auto & fix: d->updateOps.executable_fixes) {
        const auto &path = fix.first;
        bool executable = fix.second;
        auto targetPath = FS::PathCombine(dataRoot, path.toString());
        qDebug() << "FIX_EXEC" << targetPath << "->" << (executable ? "EXECUTABLE" : "REGULAR");

        auto perms = QFile::permissions(targetPath);
        if(executable) {
            perms |= QFileDevice::ExeUser | QFileDevice::ExeGroup | QFileDevice::ExeOther;
        }
        else {
            perms &= ~(QFileDevice::ExeUser | QFileDevice::ExeGroup | QFileDevice::ExeOther);
        }
        QFile::setPermissions(targetPath, perms);
    }

    if(!d->updateOps.downloads.size()) {
        emitSucceeded();
        return;
    }

    // we download.
    d->manifestDownloadJob.reset(new NetJob(QObject::tr("Download of files for %1").arg(d->packageURL)));
    connect(d->manifestDownloadJob.get(), &NetJob::succeeded, this, &PackageInstallTask::downloadsSucceeded);
    connect(d->manifestDownloadJob.get(), &NetJob::failed, this, &PackageInstallTask::downloadsFailed);
    connect(d->manifestDownloadJob.get(), &NetJob::progress, [&](qint64 current, qint64 total) {
        setProgress(current, total);
    });

    using Option = Net::Download::Option;
    for(auto & download: d->updateOps.downloads) {
        const auto &path = download.first;
        const auto &object = download.second;
        auto targetPath = FS::PathCombine(dataRoot, path.toString());
        auto dl = Net::Download::makeFile(object.url, targetPath, object.executable ? Option::SetExecutable : Option::NoOptions);
        dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, QByteArray::fromHex(object.hash.toLatin1())));
        dl->m_total_progress = object.size;
        d->manifestDownloadJob->addNetAction(dl);
        qDebug() << "DOWNLOAD" << object.url << "to" << targetPath << (object.executable ? "(EXECUTABLE)" : "(REGULAR)");
    }
    d->manifestDownloadJob->start();
}

void PackageInstallTask::downloadsFailed(QString reason)
{
    d->manifestDownloadJob.reset();
    emitFailed(reason);
}

void PackageInstallTask::downloadsSucceeded()
{
    d->manifestDownloadJob.reset();
    emitSucceeded();
}
