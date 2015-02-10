#pragma once
#include "../ProfileStrategy.h"
#include "../onesix/OneSixProfileStrategy.h"

class FTBInstance;

class FTBProfileStrategy : public OneSixProfileStrategy
{
public:
	FTBProfileStrategy(FTBInstance * instance);
	virtual ~FTBProfileStrategy() {};
	virtual void load() override;
	virtual bool saveOrder(ProfileUtils::PatchOrder order) override;
	virtual bool installJarMods(QStringList filepaths) override;
	virtual bool removePatch(PackagePtr patch) override;
};
