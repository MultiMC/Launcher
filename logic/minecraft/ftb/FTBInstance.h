#pragma once

#include "minecraft/onesix/OneSixInstance.h"

class OneSixLibrary;

class FTBInstance : public OneSixInstance
{
	Q_OBJECT
public:
	explicit FTBInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir);
    virtual ~FTBInstance(){};

	void copy(const QDir &newDir) override;

	virtual void createProfile() override;

	virtual QString getStatusbarDescription() override;

	virtual std::shared_ptr<Task> doUpdate() override;

	virtual QString id() const override;

	QString FTBLibraryPrefix() const;

	virtual QString jarModsDir() const;

private:
	SettingsObjectPtr m_globalSettings;
};
