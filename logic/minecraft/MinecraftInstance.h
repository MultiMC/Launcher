#pragma once
#include "BaseInstance.h"
#include "minecraft/Mod.h"

class ModList;

class MinecraftInstance: public BaseInstance
{
public:
	MinecraftInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir);
	virtual ~MinecraftInstance() {};

	/// Path to the instance's minecraft directory.
	QString minecraftRoot() const;

	//////  Mod Lists  //////
	virtual std::shared_ptr<ModList> resourcePackList() const
	{
		return nullptr;
	}
	virtual std::shared_ptr<ModList> texturePackList() const
	{
		return nullptr;
	}
};

typedef std::shared_ptr<MinecraftInstance> MinecraftInstancePtr;
