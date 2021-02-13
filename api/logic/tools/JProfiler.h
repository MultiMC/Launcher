#pragma once

#include "BaseProfiler.h"

#include "launcher_logic_export.h"

class LAUNCHER_LOGIC_EXPORT JProfilerFactory : public BaseProfilerFactory
{
public:
    QString name() const override { return "JProfiler"; }
    void registerSettings(SettingsObjectPtr settings) override;
    BaseExternalTool *createTool(InstancePtr instance, QObject *parent = 0) override;
    bool check(QString *error) override;
    bool check(const QString &path, QString *error) override;
};
