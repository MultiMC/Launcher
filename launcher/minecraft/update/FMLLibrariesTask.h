#pragma once
#include "tasks/Task.h"
#include "net/NetJob.h"
#include "minecraft/VersionFilterData.h"

class MinecraftInstance;

class FMLLibrariesTask : public Task
{
    Q_OBJECT
public:
    FMLLibrariesTask(MinecraftInstance * inst);
    virtual ~FMLLibrariesTask() {};

    void executeTask() override;

    bool canAbort() const override;

private slots:
    void fmllibsFinished();
    void fmllibsFailed(QString reason);

public slots:
    bool abort() override;

private:
    MinecraftInstance *m_inst;
    NetJob::Ptr downloadJob;
    QList<FMLlib> fmlLibsToProcess;
};

