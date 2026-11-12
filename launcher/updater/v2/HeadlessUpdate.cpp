#include "HeadlessUpdate.h"

#include <QCoreApplication>
#include <QDebug>

#include <iostream>

#include "FileSystem.h"
#include "InstallController.h"

namespace UpdaterV2
{

HeadlessUpdate::HeadlessUpdate(Checker *checker, shared_qobject_ptr<QNetworkAccessManager> network, const QString &site,
                               const QString &root, Tier tier, QObject *parent)
    : QObject(parent), m_checker(checker), m_network(network), m_site(site), m_root(root), m_tier(tier)
{
}

void HeadlessUpdate::start()
{
    qDebug() << "Update (headless): checking tier" << tierName(m_tier);
    connect(m_checker, &Checker::finished, this, &HeadlessUpdate::checked);
    // as switching to the tier in the settings: an older build there is installed too
    m_checker->check(m_tier, true, true);
}

void HeadlessUpdate::checked(const CheckResult &result)
{
    disconnect(m_checker, &Checker::finished, this, &HeadlessUpdate::checked);
    m_result = result;
    auto details = result.messages.join(" ");
    switch(result.kind)
    {
        case CheckResult::Update:
        case CheckResult::Switch:
        case CheckResult::Downgrade:
            break;
        case CheckResult::NoUpdate:
        case CheckResult::BehindInstalled:
            finish(NothingToDo, QString("nothing to do: build %1 is installed, tier %2 is on build %3")
                                    .arg(result.currentBuild).arg(tierName(result.tier)).arg(result.build));
            return;
        case CheckResult::NotForThisMachine:
            finish(NotForThisMachine, QString("build %1 has no variant for this machine: %2").arg(result.build).arg(details));
            return;
        case CheckResult::Failed:
        default:
            finish(Failed, QString("the check failed: %1").arg(details));
            return;
    }
    qDebug() << "Update (headless): installing build" << result.build << "variant" << result.variant.id << "over build"
             << result.currentBuild << "variant" << result.ownVariant;
    auto staging = FS::PathCombine(m_root, "update", "XXXXXX");
    if(!FS::ensureFilePathExists(staging))
    {
        finish(Failed, QString("couldn't make the folder for the update's files in %1").arg(m_root));
        return;
    }
    m_download.reset(new DownloadTask(m_network, result, m_site, m_root, staging));
    connect(m_download.get(), &Task::succeeded, this, &HeadlessUpdate::downloaded);
    connect(m_download.get(), &Task::failed, this, &HeadlessUpdate::downloadFailed);
    m_download->start();
}

void HeadlessUpdate::downloadFailed(const QString &reason)
{
    finish(Failed, QString("the download failed: %1").arg(reason));
}

void HeadlessUpdate::downloaded()
{
    // restarts the new build with these arguments and --alive, and waits for it to start
    InstallController install(nullptr, m_root, m_download->stagingDir(), m_download->changes(), false);
    install.installUpdates();
    if(!install.succeeded())
    {
        finish(Failed, QString("the install failed: %1").arg(install.error()));
        return;
    }
    finish(Updated, QString("updated from build %1 to build %2 (%3)").arg(m_result.currentBuild).arg(m_result.build).arg(m_result.variant.id));
}

void HeadlessUpdate::finish(Status status, const QString &message)
{
    auto line = QString("Update (headless): %1, exit status %2").arg(message).arg(int(status));
    if(status == Updated || status == NothingToDo)
    {
        qDebug().noquote() << line;
    }
    else
    {
        qWarning().noquote() << line;
    }
    std::cout << line.toStdString() << std::endl;
    QCoreApplication::exit(int(status));
}

}
