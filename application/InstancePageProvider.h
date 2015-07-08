#pragma once
#include "minecraft/OneSixInstance.h"
#include "minecraft/LegacyInstance.h"
#include "pages/BasePage.h"
#include "pages/VersionPage.h"
#include "pages/ModFolderPage.h"
#include "pages/ResourcePackPage.h"
#include "pages/TexturePackPage.h"
#include "pages/NotesPage.h"
#include "pages/ScreenshotsPage.h"
#include "pages/InstanceSettingsPage.h"
#include "pages/OtherLogsPage.h"
#include "pages/BasePageProvider.h"
#include "pages/LegacyJarModPage.h"
#include "pages/global/AccountListPage.h"
#include <pathutils.h>


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
		std::shared_ptr<OneSixInstance> onesix = std::dynamic_pointer_cast<OneSixInstance>(inst);
		if(onesix)
		{
			values.append(new VersionPage(onesix.get()));
			auto modsPage = new ModFolderPage(onesix.get(), onesix->loaderModList(), "mods", "loadermods", tr("Loader mods"), "Loader-mods");
			modsPage->setFilter(tr("%1 (*.zip *.jar *.litemod)"));
			values.append(modsPage);
			values.append(new CoreModFolderPage(onesix.get(), onesix->coreModList(), "coremods", "coremods", tr("Core mods"), "Core-mods"));
			values.append(new ResourcePackPage(onesix.get()));
			values.append(new TexturePackPage(onesix.get()));
			values.append(new NotesPage(onesix.get()));
			values.append(new ScreenshotsPage(PathCombine(onesix->minecraftRoot(), "screenshots")));
			values.append(new InstanceSettingsPage(onesix.get()));
			values.append(new OtherLogsPage(onesix->minecraftRoot()));
			values.append(new AccountListPage(inst));
		}
		std::shared_ptr<LegacyInstance> legacy = std::dynamic_pointer_cast<LegacyInstance>(inst);
		if(legacy)
		{
			QList<BasePage *> values;
			// FIXME: actually implement the legacy instance upgrade, then enable this.
			//values.append(new LegacyUpgradePage(this));
			values.append(new LegacyJarModPage(legacy.get()));
			auto modsPage = new ModFolderPage(legacy.get(), legacy->loaderModList(), "mods", "loadermods", tr("Loader mods"), "Loader-mods");
			modsPage->setFilter(tr("%1 (*.zip *.jar *.litemod)"));
			values.append(modsPage);
			values.append(new ModFolderPage(legacy.get(), legacy->coreModList(), "coremods", "coremods", tr("Core mods"), "Loader-mods"));
			values.append(new TexturePackPage(legacy.get()));
			values.append(new NotesPage(legacy.get()));
			values.append(new ScreenshotsPage(PathCombine(legacy->minecraftRoot(), "screenshots")));
			values.append(new InstanceSettingsPage(legacy.get()));
			values.append(new OtherLogsPage(legacy->minecraftRoot()));
			values.append(new AccountListPage(inst));
			return values;
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
