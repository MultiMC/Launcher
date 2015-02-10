#pragma once

#include "ProfileStrategy.h"

class NullProfileStrategy: public ProfileStrategy
{
	virtual bool installJarMods(QStringList filepaths)
	{
		return false;
	}
	virtual void load() {};
	virtual bool removePatch(PackagePtr jarMod)
	{
		return false;
	}
	virtual bool saveOrder(ProfileUtils::PatchOrder order)
	{
		return false;
	}
};