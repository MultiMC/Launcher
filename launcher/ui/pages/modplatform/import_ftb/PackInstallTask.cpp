/*
 * Copyright 2022 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "PackInstallTask.h"

#include <QtConcurrent>

#include "MMCZip.h"
#include "BaseInstance.h"
#include "FileSystem.h"
#include "settings/INISettingsObject.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/GradleSpecifier.h"

#include "BuildConfig.h"
#include "Application.h"

#include <pathmatcher/RegexpMatcher.h>

namespace ImportFTB {

PackInstallTask::PackInstallTask(Modpack pack) {
    m_pack = pack;
}

void PackInstallTask::executeTask() {
    FS::copy folderCopy(m_pack.instanceDir, FS::PathCombine(m_stagingPath, ".minecraft"));
    folderCopy.followSymlinks(true).blacklist(new RegexpMatcher("(instance|version)[.]json"));

    m_copyFuture = QtConcurrent::run(QThreadPool::globalInstance(), folderCopy);
    connect(&m_copyFutureWatcher, &QFutureWatcher<bool>::finished, this, &PackInstallTask::copyFinished);
    connect(&m_copyFutureWatcher, &QFutureWatcher<bool>::canceled, this, &PackInstallTask::copyAborted);
    m_copyFutureWatcher.setFuture(m_copyFuture);
    abortable = true;
}

void PackInstallTask::copyFinished() {
    abortable = false;
    QString instanceConfigPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(instanceConfigPath);
    instanceSettings->suspendSave();
    instanceSettings->registerSetting("InstanceType", "Legacy");
    instanceSettings->set("InstanceType", "OneSix");

    MinecraftInstance instance(m_globalSettings, instanceSettings, m_stagingPath);
    auto components = instance.getPackProfile();
    components->buildingFromScratch();
    components->setComponentVersion("net.minecraft", m_pack.mcVersion, true);

    auto modloader = m_pack.modLoader;
    switch(modloader.type) {
        case ModLoaderType::None: {
            // NOOP
            break;
        }
        case ModLoaderType::Forge: {
            components->setComponentVersion("net.minecraftforge", modloader.version, true);
            break;
        }
        case ModLoaderType::NeoForge: {
            components->setComponentVersion("net.neoforged", modloader.version, true);
            break;
        }
        case ModLoaderType::Fabric: {
            components->setComponentVersion("net.fabricmc.fabric-loader", modloader.version, true);
            break;
        }
        case ModLoaderType::Quilt: {
            components->setComponentVersion("org.quiltmc.quilt-loader", modloader.version, true);
            break;
        }
        case ModLoaderType::Unresolved: {
            qWarning() << "Unknown modloader version: " << modloader.id;
        }
    }
    components->saveNow();

    instance.setName(m_instName);
    if(m_instIcon == "default")
    {
        m_instIcon = "ftb_logo";
    }
    instance.setIconKey(m_instIcon);
    instanceSettings->resumeSave();

    emitSucceeded();
}

void PackInstallTask::copyAborted() {
    emitAborted();
}


bool ImportFTB::PackInstallTask::canAbort() const {
    return abortable;
}

bool ImportFTB::PackInstallTask::abort() {
    if(abortable) {
        m_copyFuture.cancel();
        return true;
    }
    return false;
}

}
