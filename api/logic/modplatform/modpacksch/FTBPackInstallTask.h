#pragma once

#include "FTBPackManifest.h"

#include "InstanceTask.h"
#include "multimc_logic_export.h"
#include "net/NetJob.h"

namespace ModpacksCH {

struct MULTIMC_LOGIC_EXPORT FlaggedMod {
    QString fileName;

    /**
     * The name of the mod.
     */
    QString modName;
    /**
     * A description of the mod's purpose, and why a user may want to enable the mod.
     */
    QString modDescription;

    /**
     * The reason the mod was flagged, this should be plain-and-simple.
     * <p>
     * Examples may include: 'Tracking' or 'Advertising'
     */
    QString flagReason;
};

class MULTIMC_LOGIC_EXPORT UserInteractionSupport {

public:
    virtual void showFlaggedModsDialog(QVector<FlaggedMod> mods) = 0;

};

class MULTIMC_LOGIC_EXPORT PackInstallTask : public InstanceTask
{
    Q_OBJECT

public:
    explicit PackInstallTask(UserInteractionSupport *support, Modpack pack, QString version);
    virtual ~PackInstallTask(){}

    bool abort() override;

protected:
    virtual void executeTask() override;

private slots:
    void onDownloadSucceeded();
    void onDownloadFailed(QString reason);

private:
    void downloadPack();
    void install();

private:
    UserInteractionSupport *m_support;

    NetJobPtr jobPtr;
    QByteArray response;

    Modpack m_pack;
    QString m_version_name;
    Version m_version;

};

}
