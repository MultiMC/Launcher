#pragma once

#include "tasks/Task.h"
#include "launcher_logic_export.h"
#include "settings/SettingsObject.h"

class LAUNCHER_LOGIC_EXPORT InstanceTask : public Task
{
    Q_OBJECT
public:
    explicit InstanceTask();
    virtual ~InstanceTask();

    void setParentSettings(SettingsObjectPtr settings)
    {
        m_globalSettings = settings;
    }

    void setStagingPath(const QString &stagingPath)
    {
        m_stagingPath = stagingPath;
    }

    void setName(const QString &name)
    {
        m_instName = name;
    }
    QString name() const
    {
        return m_instName;
    }

    void setIcon(const QString &icon)
    {
        m_instIcon = icon;
    }

    void setGroup(const QString &group)
    {
        m_instGroup = group;
    }
    QString group() const
    {
        return m_instGroup;
    }

protected: /* data */
    SettingsObjectPtr m_globalSettings;
    QString m_instName;
    QString m_instIcon;
    QString m_instGroup;
    QString m_stagingPath;
};
