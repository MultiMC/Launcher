#include <QtXml/QDomDocument>
#include <Env.h>
#include <quazip.h>
#include <QtConcurrent/QtConcurrent>
#include <MMCZip.h>
#include <minecraft/OneSixVersionFormat.h>
#include "ATLPackInstallTask.h"

#include "BuildConfig.h"
#include "FileSystem.h"
#include "Json.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/INISettingsObject.h"
#include "meta/Index.h"
#include "meta/VersionList.h"

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
    auto searchUrl = QString(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "packs/%1/versions/%2/Configs.xml")
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

    if(m_version.pack.noConfigs) {
        installMods();
    }
    else {
        installConfigs();
    }
}

void PackInstallTask::onDownloadFailed(QString reason)
{
    jobPtr.reset();
    emitFailed(reason);
}

QString PackInstallTask::getDirForModType(ModType type, QString raw)
{
    switch (type) {
        case ModType::Root:
        case ModType::Extract:
            // Handled elsewhere
            return Q_NULLPTR;
        case ModType::MCPC:
            // we can safely ignore MCPC server jar
            return Q_NULLPTR;
        case ModType::Forge:
            // todo: detect Forge version and install through a proper component
        case ModType::Jar:
            return "jarmods";
        case ModType::Mods:
            return "mods";
        case ModType::Flan:
            return "Flan";
        case ModType::Dependency:
            return FS::PathCombine("mods", m_version.pack.minecraft);
        case ModType::Ic2Lib:
            return FS::PathCombine("mods", "ic2");
        case ModType::DenLib:
            return FS::PathCombine("mods", "denlib");
        case ModType::Coremods:
            return "coremods";
        case ModType::Plugins:
            return "plugins";
        case ModType::Decomp:
            qWarning() << "Unsupported mod type: " + raw;
            return Q_NULLPTR;
        case ModType::TexturePack:
            return "texturepackss";
        case ModType::ResourcePack:
            return "resourcepacks";
        case ModType::ShaderPack:
            return "shaderpacks";
        case ModType::Unknown:
            emitFailed(tr("Unknown mod type: ") + raw);
            return Q_NULLPTR;
    }

    return Q_NULLPTR;
}

QString PackInstallTask::getVersionForLoader(QString uid)
{
    if(m_version.loader.recommended || m_version.loader.latest || m_version.loader.choose) {
        auto vlist = ENV.metadataIndex()->get(uid);
        if(!vlist)
        {
            emitFailed(tr("Failed to get local metadata index for ") + uid);
            return Q_NULLPTR;
        }

        // todo: filter by Minecraft version

        if(m_version.loader.recommended) {
            return vlist.get()->getRecommended().get()->descriptor();
        }
        else if(m_version.loader.latest) {
            return vlist.get()->at(0)->descriptor();
        }
        else if(m_version.loader.choose) {
            // todo: implement
        }
    }

    return m_version.loader.version;
}

bool PackInstallTask::createLibrariesComponent(QString instanceRoot, std::shared_ptr<PackProfile> profile)
{
    if(m_version.libraries.isEmpty()) {
        return true;
    }

    auto uuid = QUuid::createUuid();
    auto id = uuid.toString().remove('{').remove('}');
    auto target_id = "org.multimc.atlauncher." + id;

    auto patchDir = FS::PathCombine(instanceRoot, "patches");
    if(!FS::ensureFolderPathExists(patchDir))
    {
        return false;
    }
    auto patchFileName = FS::PathCombine(patchDir, target_id + ".json");

    auto f = std::make_shared<VersionFile>();
    f->name = m_pack + " " + m_version_name + " (libraries)";

    for(auto lib : m_version.libraries) {
        auto library = std::make_shared<Library>();
        library->setRawName("org.multimc.atlauncher:" + lib.md5 + ":1");

        switch(lib.download) {
            case DownloadType::Server:
                library->setAbsoluteUrl(BuildConfig.ATL_DOWNLOAD_SERVER_URL + lib.url);
                break;
            case DownloadType::Direct:
                library->setAbsoluteUrl(lib.url);
                break;
            case DownloadType::Browser:
            case DownloadType::Unknown:
                emitFailed(tr("Unknown or unsupported download type: ") + lib.download_raw);
                return false;
        }

        f->libraries.append(library);
    }

    QFile file(patchFileName);
    if (!file.open(QFile::WriteOnly))
    {
        qCritical() << "Error opening" << file.fileName()
                    << "for reading:" << file.errorString();
        return false;
    }
    file.write(OneSixVersionFormat::versionFileToJson(f).toJson());
    file.close();

    profile->appendComponent(new Component(profile.get(), target_id, f));
    return true;
}

bool PackInstallTask::createPackComponent(QString instanceRoot, std::shared_ptr<PackProfile> profile)
{
    if(!(m_version.pack.mainClass != QString() || m_version.pack.extraArguments != QString())) {
        return true;
    }

    auto uuid = QUuid::createUuid();
    auto id = uuid.toString().remove('{').remove('}');
    auto target_id = "org.multimc.atlauncher." + id;

    auto patchDir = FS::PathCombine(instanceRoot, "patches");
    if(!FS::ensureFolderPathExists(patchDir))
    {
        return false;
    }
    auto patchFileName = FS::PathCombine(patchDir, target_id + ".json");

    auto f = std::make_shared<VersionFile>();
    f->name = m_pack + " " + m_version_name;
    f->mainClass = m_version.pack.mainClass;

    // Parse out tweakers
    auto args = m_version.pack.extraArguments.split(" ");
    QString previous;
    for(auto arg : args) {
        if(arg.startsWith("--tweakClass=") || previous == "--tweakClass") {
            auto tweakClass = arg.remove("--tweakClass=");
            f->addTweakers.append(tweakClass);
        }
        previous = arg;
    }

    QFile file(patchFileName);
    if (!file.open(QFile::WriteOnly))
    {
        qCritical() << "Error opening" << file.fileName()
                    << "for reading:" << file.errorString();
        return false;
    }
    file.write(OneSixVersionFormat::versionFileToJson(f).toJson());
    file.close();

    profile->appendComponent(new Component(profile.get(), target_id, f));
    return true;
}

void PackInstallTask::installConfigs()
{
    setStatus(tr("Downloading configs..."));
    jobPtr.reset(new NetJob(tr("Config download")));

    auto path = QString("Configs/%1/%2").arg(m_pack).arg(m_version_name);
    auto url = QString(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "packs/%1/versions/%2/Configs.zip")
            .arg(m_pack).arg(m_version_name);
    auto entry = ENV.metacache()->resolveEntry("ATLauncherPacks", path);
    entry->setStale(true);

    jobPtr->addNetAction(Net::Download::makeCached(url, entry));
    archivePath = entry->getFullPath();

    connect(jobPtr.get(), &NetJob::succeeded, this, [&]()
    {
        jobPtr.reset();
        extractConfigs();
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

    jobPtr->start();
}

void PackInstallTask::extractConfigs()
{
    setStatus(tr("Extracting configs..."));

    QDir extractDir(m_stagingPath);

    QuaZip packZip(archivePath);
    if(!packZip.open(QuaZip::mdUnzip))
    {
        emitFailed(tr("Failed to open pack configs %1!").arg(archivePath));
        return;
    }

    m_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), MMCZip::extractDir, archivePath, extractDir.absolutePath() + "/minecraft");
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, [&]()
    {
        installMods();
    });
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, [&]()
    {
        emitAborted();
    });
    m_extractFutureWatcher.setFuture(m_extractFuture);
}

void PackInstallTask::installMods()
{
    setStatus(tr("Downloading mods..."));

    jarmods.clear();
    jobPtr.reset(new NetJob(tr("Mod download")));
    for(const auto& mod : m_version.mods) {
        // skip optional mods for now
        if(mod.optional) continue;

        QString url;
        switch(mod.download) {
            case DownloadType::Server:
                url = BuildConfig.ATL_DOWNLOAD_SERVER_URL + mod.url;
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

        if (mod.type == ModType::Extract) {
            auto entry = ENV.metacache()->resolveEntry("ATLauncherPacks", mod.url);
            entry->setStale(true);
            modsToExtract.insert(entry->getFullPath(), mod);

            auto dl = Net::Download::makeCached(url, entry);
            jobPtr->addNetAction(dl);
        }
        else {
            auto relpath = getDirForModType(mod.type, mod.type_raw);
            if(relpath == Q_NULLPTR) continue;
            auto path = FS::PathCombine(m_stagingPath, "minecraft", relpath, mod.file);

            qDebug() << "Will download" << url << "to" << path;
            auto dl = Net::Download::makeFile(url, path);
            jobPtr->addNetAction(dl);

            if(mod.type == ModType::Jar || mod.type == ModType::Forge) {
                qDebug() << "Jarmod: " + path;
                jarmods.push_back(path);
            }
        }
    }

    connect(jobPtr.get(), &NetJob::succeeded, this, [&]()
    {
        jobPtr.reset();
        extractMods();
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

    jobPtr->start();
}

void PackInstallTask::extractMods()
{
    setStatus(tr("Extracting mods..."));

    if(modsToExtract.isEmpty()) {
        install();
        return;
    }

    auto modPath = modsToExtract.firstKey();
    auto mod = modsToExtract.value(modPath);

    auto extractToDir = getDirForModType(mod.extractTo, mod.extractTo_raw);
    qDebug() << "Extracting " + mod.file + " to " + extractToDir;

    QDir extractDir(m_stagingPath);
    auto extractToPath = FS::PathCombine(extractDir.absolutePath(), "minecraft", extractToDir);

    m_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), MMCZip::extractDir, modPath, mod.extractFolder, extractToPath);
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, [&]()
    {
        extractMods();
    });
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, [&]()
    {
        emitAborted();
    });
    m_extractFutureWatcher.setFuture(m_extractFuture);

    modsToExtract.remove(modPath);
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

    // Use a component to add libraries BEFORE Minecraft
    if(!createLibrariesComponent(instance.instanceRoot(), components)) {
        return;
    }

    // Minecraft
    components->setComponentVersion("net.minecraft", m_version.pack.minecraft, true);

    // Loader
    if(m_version.loader.type == QString("forge"))
    {
        auto version = getVersionForLoader("net.minecraftforge");
        if(version == Q_NULLPTR) return;

        components->setComponentVersion("net.minecraftforge", version, true);
    }
    else if(m_version.loader.type == QString("fabric"))
    {
        auto version = getVersionForLoader("net.fabricmc.fabric-loader");
        if(version == Q_NULLPTR) return;

        components->setComponentVersion("net.fabricmc.fabric-loader", version, true);
    }
    else if(m_version.loader.type != QString())
    {
        emitFailed(tr("Unknown loader type: ") + m_version.loader.type);
        return;
    }

    components->installJarMods(jarmods);

    // Use a component to fill in the rest of the data
    // todo: use more detection
    if(!createPackComponent(instance.instanceRoot(), components)) {
        return;
    }

    components->saveNow();

    instance.setName(m_instName);
    instance.setIconKey(m_instIcon);
    instanceSettings->resumeSave();

    jarmods.clear();
    emitSucceeded();
}

}
