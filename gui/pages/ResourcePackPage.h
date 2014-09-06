#pragma once
#include "ModFolderPage.h"

class ResourcePackPage : public ModFolderPage
{
public:
	explicit ResourcePackPage(BaseInstance *instance, QWidget *parent = 0)
		: ModFolderPage(QuickModInstanceModList::ResourcePacks, instance, instance->resourcePackList(), "resourcepacks",
						"resourcepacks", tr("Resource packs"), "Resource-packs", parent)
	{
	}

	virtual ~ResourcePackPage() {}
	virtual bool shouldDisplay() const override
	{
		return !m_inst->traits().contains("no-texturepacks") &&
			   !m_inst->traits().contains("texturepacks");
	}
};
