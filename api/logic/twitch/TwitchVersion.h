#pragma once

#include <QString>

#include "BaseVersion.h"
#include "TwitchData.h"

struct TwitchVersion : public BaseVersion
{
    TwitchVersion(){}
    TwitchVersion(const Twitch::AddonFile &addon)
    : m_addon(addon)
    {
    }
    virtual QString descriptor()
    {
        return QString::number(m_addon.fileId);
    }

    virtual QString name()
    {
        return m_addon.version;
    }

    virtual QString typeString() const
    {
        return m_addon.mcVersion;
    }

    QString getURL()
    {
         return m_addon.downloadUrl;
    }

    bool operator<(const TwitchVersion & rhs);
    bool operator==(const TwitchVersion & rhs);
    bool operator>(const TwitchVersion & rhs);

    Twitch::AddonFile m_addon;

    bool recommended = false;
};

typedef std::shared_ptr<TwitchVersion> TwitchVersionPtr;
