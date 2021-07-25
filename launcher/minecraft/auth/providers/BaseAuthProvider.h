#pragma once

#include <QObject>
#include "QObjectPtr.h"
#include <QDateTime>
#include <QSet>
#include <QProcess>
#include <QUrl>


#include "logic_export.h"
#include "minecraft/auth/AccountProfile.h"

class BaseAuthProvider;
typedef std::shared_ptr<BaseAuthProvider> AuthProviderPtr;

/*!
 * \brief Base class for auth provider.
 * This class implements many functions that are common between providers and
 * provides a standard interface for all providers.
 *
 * To create a new provider, create a new class inheriting from this class,
 * implement the pure virtual functions, and
 */
class LOGIC_EXPORT BaseAuthProvider : public QObject
{
    Q_OBJECT

public:
    virtual ~BaseAuthProvider(){};

    // Unique id for provider
    virtual QString id()
    {
        return "base";
    };

    // Name of provider that displayed in account selector and list
    virtual QString displayName()
    {
        return "Base";
    };

    // Use dummy auth on login instead of calling endpoint
    virtual bool dummyAuth()
    {
        return false;
    };

    // Endpoint for authlib injector (use empty if authlib injector isn't required)
    virtual QString injectorEndpoint()
    {
        return "";
    };

    // Endpoint for authentication
    virtual QString authEndpoint()
    {
        return "";
    };

    // Function to get url of skin to display in launcher
    virtual QUrl resolveSkinUrl(AccountProfile profile)
    {
        return QUrl(((QString) "https://crafatar.com/skins/%1.png").arg(profile.id));
    };

    // Can change skin (currently only mojang support)
    virtual bool canChangeSkin()
    {
        return false;
    }
};
