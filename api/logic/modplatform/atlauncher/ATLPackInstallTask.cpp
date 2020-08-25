#include <QtXml/QDomDocument>
#include "ATLPackInstallTask.h"

#include "BuildConfig.h"
#include "FileSystem.h"
#include "Json.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/INISettingsObject.h"

namespace ATLauncher {

PackInstallTask::PackInstallTask(QString pack, QString version)
{
    m_pack = pack;
    m_version_name = version;
}

bool PackInstallTask::abort()
{
    return true;
}

void PackInstallTask::executeTask()
{
    auto *netJob = new NetJob("ATLauncher::VersionFetch");
    auto searchUrl = QString(BuildConfig.ATL_DOWNLOAD_SERVER + "packs/%1/versions/%2/Configs.xml")
            .arg(m_pack).arg(m_version_name);
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &PackInstallTask::onDownloadSucceeded);
    QObject::connect(netJob, &NetJob::failed, this, &PackInstallTask::onDownloadFailed);
}

void PackInstallTask::onDownloadSucceeded()
{
    jobPtr.reset();

    QDomDocument doc;

    QString errorMsg = "Unknown error.";
    int errorLine = -1;
    int errorCol = -1;

    if(!doc.setContent(response, false, &errorMsg, &errorLine, &errorCol))
    {
        auto fullErrMsg = QString("Failed to fetch modpack data: %1 %2:3d!").arg(errorMsg, errorLine, errorCol);
        qWarning() << fullErrMsg;
        response.clear();
        return;
    }

    ATLauncher::Version version;
    ATLauncher::loadVersion(version, doc);
    m_version = version;

    install();
}

void PackInstallTask::onDownloadFailed(QString reason)
{
    jobPtr.reset();
    emitFailed(reason);
}

void PackInstallTask::install()
{
    setStatus(tr("Installing modpack"));

    auto instanceConfigPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(instanceConfigPath);
    instanceSettings->registerSetting("InstanceType", "Legacy");
    instanceSettings->set("InstanceType", "OneSix");

    MinecraftInstance instance(m_globalSettings, instanceSettings, m_stagingPath);
    auto components = instance.getPackProfile();
    components->buildingFromScratch();

    // Minecraft
    components->setComponentVersion("net.minecraft", m_version.pack.minecraft, true);

    // Loader
    if(m_version.loader.type == QString("forge"))
    {
        components->setComponentVersion("net.minecraftforge", m_version.loader.version, true);
    }
    else if(m_version.loader.type == QString("fabric"))
    {
        components->setComponentVersion("net.fabricmc.fabric-loader", m_version.loader.version, true);
    }
    else if(m_version.loader.type != QString())
    {
        emitFailed(tr("Unknown loader type: ") + m_version.loader.type);
        return;
    }
    components->saveNow();

    jobPtr.reset(new NetJob(tr("Mod download")));
    QStringList jarmods;
    for(const auto& mod : m_version.mods) {
        QString relpath;
        switch(mod.type) {
            case ModType::Forge:
                // todo: detect Forge version and install through a proper component
            case ModType::Jar:
                jarmods.push_back(mod.file);
                relpath = "jarmods";
                break;
            case ModType::Mods:
                relpath = FS::PathCombine("minecraft", "mods");
                break;
            case ModType::Flan:
                relpath = FS::PathCombine("minecraft", "Flan");
                break;
            case ModType::Dependency:
                relpath = FS::PathCombine("minecraft", "mods", m_version.pack.minecraft);
                break;
            case ModType::Ic2Lib:
                relpath = FS::PathCombine("minecraft", "mods", "ic2");
                break;
            case ModType::DenLib:
                relpath = FS::PathCombine("minecraft", "mods", "denlib");
                break;
            case ModType::Coremods:
                relpath = FS::PathCombine("minecraft", "coremods");
                break;
            case ModType::MCPC:
                // we can safely ignore MCPC server jar
                break;
            case ModType::Plugins:
                relpath = FS::PathCombine("minecraft", "plugins");
                break;
            case ModType::Extract:
            case ModType::Decomp:
                // todo(merged): fail hard
                qWarning() << "Unsupported mod type: " + mod.type_raw;
                continue;
            case ModType::ResourcePack:
                relpath = FS::PathCombine("minecraft", "resourcepacks");
                break;
            case ModType::Unknown:
                emitFailed(tr("Unknown mod type: ") + mod.type_raw);
                return;
        }
        auto path = FS::PathCombine(m_stagingPath, relpath, mod.file);

        QString url;
        switch(mod.download) {
            case DownloadType::Server:
                url = BuildConfig.ATL_DOWNLOAD_SERVER + mod.url;
                break;
            case DownloadType::Browser:
                emitFailed(tr("Unsupported download type: ") + mod.download_raw);
                return;
            case DownloadType::Direct:
                url = mod.url;
                break;
            case DownloadType::Unknown:
                emitFailed(tr("Unknown download type: ") + mod.download_raw);
                return;
        }

        qDebug() << "Will download" << url << "to" << path;
        auto dl = Net::Download::makeFile(url, path);
        jobPtr->addNetAction(dl);
    }
    components->installJarMods(jarmods);

    connect(jobPtr.get(), &NetJob::succeeded, this, [&]()
    {
        jobPtr.reset();
        emitSucceeded();
    });
    connect(jobPtr.get(), &NetJob::failed, [&](QString reason)
    {
        jobPtr.reset();
        emitFailed(reason);
    });
    connect(jobPtr.get(), &NetJob::progress, [&](qint64 current, qint64 total)
    {
        setProgress(current, total);
    });

    setStatus(tr("Downloading mods..."));
    jobPtr->start();

    instance.setName(m_instName);
    instance.setIconKey(m_instIcon);
    instanceSettings->resumeSave();
}

}
