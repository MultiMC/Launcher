#pragma once
#include "minecraft/MinecraftInstance.h"
#include "minecraft/legacy/LegacyInstance.h"
#include <FileSystem.h>
#include "pages/BasePage.h"
#include "pages/BasePageProvider.h"
#include "pages/instance/LogPage.h"
#include "pages/instance/VersionPage.h"
#include "pages/instance/ModFolderPage.h"
#include "pages/instance/ResourcePackPage.h"
#include "pages/instance/TexturePackPage.h"
#include "pages/instance/NotesPage.h"
#include "pages/instance/ScreenshotsPage.h"
#include "pages/instance/InstanceSettingsPage.h"
#include "pages/instance/OtherLogsPage.h"
#include "pages/instance/LegacyUpgradePage.h"
#include "pages/instance/WorldListPage.h"
#include "pages/instance/ServersPage.h"
#include "pages/instance/GameOptionsPage.h"

#include "Env.h"

class InstancePageProvider : public QObject, public BasePageProvider
{
    Q_OBJECT
public:
    explicit InstancePageProvider(InstancePtr parent)
    {
        inst = parent;
    }

    virtual ~InstancePageProvider() {};
    virtual QList<BasePage *> getPages() override
    {
        QList<BasePage *> values;
        values.append(new LogPage(inst));
        std::shared_ptr<MinecraftInstance> onesix = std::dynamic_pointer_cast<MinecraftInstance>(inst);
        if(onesix)
        {
            values.append(new VersionPage(onesix.get()));
            auto modsPage = new ModFolderPage(onesix.get(), onesix->loaderModList(), "mods", "loadermods", tr("Loader mods"), "Loader-mods");
            modsPage->setFilter("%1 (*.zip *.jar *.litemod)");
            values.append(modsPage);
            values.append(new CoreModFolderPage(onesix.get(), onesix->coreModList(), "coremods", "coremods", tr("Core mods"), "Core-mods"));
            values.append(new ResourcePackPage(onesix.get()));
            values.append(new TexturePackPage(onesix.get()));
            values.append(new NotesPage(onesix.get()));
            values.append(new WorldListPage(onesix.get(), onesix->worldList()));
            values.append(new ServersPage(onesix));
            // values.append(new GameOptionsPage(onesix.get()));
            values.append(new ScreenshotsPage(FS::PathCombine(onesix->gameRoot(), "screenshots")));
            values.append(new InstanceSettingsPage(onesix.get()));
        }
        std::shared_ptr<LegacyInstance> legacy = std::dynamic_pointer_cast<LegacyInstance>(inst);
        if(legacy)
        {
            values.append(new LegacyUpgradePage(legacy));
            values.append(new NotesPage(legacy.get()));
            values.append(new WorldListPage(legacy.get(), legacy->worldList()));
            values.append(new ScreenshotsPage(FS::PathCombine(legacy->gameRoot(), "screenshots")));
        }
        auto logMatcher = inst->getLogFileMatcher();
        if(logMatcher)
        {
            values.append(new OtherLogsPage(inst->getLogFileRoot(), logMatcher));
        }
        return values;
    }

    virtual QString dialogTitle() override
    {
        return tr("Edit Instance (%1)").arg(inst->name());
    }
protected:
    InstancePtr inst;
};
