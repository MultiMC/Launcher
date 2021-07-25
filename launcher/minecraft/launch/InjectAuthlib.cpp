#include "InjectAuthlib.h"
#include <launch/LaunchTask.h>
#include <minecraft/MinecraftInstance.h>
#include <FileSystem.h>
#include <Env.h>
#include <Json.h>

InjectAuthlib::InjectAuthlib(LaunchTask *parent, AuthlibInjectorPtr* injector) : LaunchStep(parent)
{
    m_injector = injector;
}

void InjectAuthlib::executeTask()
{
    if (m_aborted)
    {
        emitFailed(tr("Task aborted."));
        return;
    }

    auto latestVersionInfo = QString("https://authlib-injector.yushi.moe/artifact/latest.json");
    auto netJob = new NetJob("Injector versions info download");
    MetaEntryPtr entry = ENV.metacache()->resolveEntry("injectors", "version.json");
    if (!m_offlineMode)
    {
        entry->setStale(true);
        auto task = Net::Download::makeCached(QUrl(latestVersionInfo), entry);
        netJob->addNetAction(task);

        jobPtr.reset(netJob);
        QObject::connect(netJob, &NetJob::succeeded, this, &InjectAuthlib::onVersionDownloadSucceeded);
        QObject::connect(netJob, &NetJob::failed, this, &InjectAuthlib::onDownloadFailed);
        jobPtr->start();
    }
    else
    {
        onVersionDownloadSucceeded();
    }
}

void InjectAuthlib::onVersionDownloadSucceeded()
{

    QByteArray data;
    try
    {
        data = FS::read(QDir("injectors").absoluteFilePath("version.json"));
    }
    catch (const Exception &e)
    {
        qCritical() << "Translations Download Failed: index file not readable";
        jobPtr.reset();
        emitFailed("Error while parsing JSON response from InjectorEndpoint");
        return;
    }

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parse_error);
    if (parse_error.error != QJsonParseError::NoError)
    {
        qCritical() << "Error while parsing JSON response from InjectorEndpoint at " << parse_error.offset << " reason: " << parse_error.errorString();
        qCritical() << data;
        jobPtr.reset();
        emitFailed("Error while parsing JSON response from InjectorEndpoint");
        return;
    }

    if (!doc.isObject())
    {
        qCritical() << "Error while parsing JSON response from InjectorEndpoint root is not object";
        qCritical() << data;
        jobPtr.reset();
        emitFailed("Error while parsing JSON response from InjectorEndpoint");
        return;
    }

    QString downloadUrl;
    try
    {
        downloadUrl = Json::requireString(doc.object(), "download_url");
    }
    catch (const JSONValidationError &e)
    {
        qCritical() << "Error while parsing JSON response from InjectorEndpoint download url is not string";
        qCritical() << e.cause();
        qCritical() << data;
        jobPtr.reset();
        emitFailed("Error while parsing JSON response from InjectorEndpoint");
        return;
    }

    QFileInfo fi(downloadUrl);
    m_versionName = fi.fileName();

    qDebug() << "Authlib injector version:" << m_versionName;
    if (!m_offlineMode)
    {
        auto netJob = new NetJob("Injector download");
        MetaEntryPtr entry = ENV.metacache()->resolveEntry("injectors", m_versionName);
        entry->setStale(true);
        auto task = Net::Download::makeCached(QUrl(downloadUrl), entry);
        netJob->addNetAction(task);

        jobPtr.reset(netJob);
        QObject::connect(netJob, &NetJob::succeeded, this, &InjectAuthlib::onDownloadSucceeded);
        QObject::connect(netJob, &NetJob::failed, this, &InjectAuthlib::onDownloadFailed);
        jobPtr->start();
    }
    else
    {
        onDownloadSucceeded();
    }
}

void InjectAuthlib::onDownloadSucceeded()
{
    QString injector = QString("-javaagent:%1=%2").arg(QDir("injectors").absoluteFilePath(m_versionName)).arg(m_authServer);

    qDebug()
        << "Injecting " << injector;
    auto inj = new AuthlibInjector(injector);
    m_injector->reset(inj);

    jobPtr.reset();
    emitSucceeded();
}

void InjectAuthlib::onDownloadFailed(QString reason)
{
    jobPtr.reset();
    emitFailed(reason);
}

void InjectAuthlib::proceed()
{
}

bool InjectAuthlib::canAbort() const
{
    if (jobPtr)
    {
        return jobPtr->canAbort();
    }
    return true;
}

bool InjectAuthlib::abort()
{
    m_aborted = true;
    if (jobPtr)
    {
        if (jobPtr->canAbort())
        {
            return jobPtr->abort();
        }
    }
    return true;
}
