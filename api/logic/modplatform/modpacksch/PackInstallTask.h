#pragma once

#include "InstanceTask.h"
#include "multimc_logic_export.h"
#include "PackManifest.h"

namespace ModpacksCH {

class MULTIMC_LOGIC_EXPORT PackInstallTask : public InstanceTask
{
    Q_OBJECT

public:
    explicit PackInstallTask(Modpack pack, QString version);
    virtual ~PackInstallTask(){}

    bool abort() override;

protected:
    virtual void executeTask() override;

private:
    Modpack m_pack;
    QString m_version;

};

}
