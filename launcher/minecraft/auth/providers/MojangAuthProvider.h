#pragma once

#include <QObject>
#include "QObjectPtr.h"
#include <QDateTime>
#include <QSet>
#include <QProcess>
#include <QDebug>

#include "BaseAuthProvider.h"

class MojangAuthProvider : public BaseAuthProvider
{
    Q_OBJECT

public:
    QString id()
    {
        return "mojang";
    }

    QString displayName()
    {
        return "Mojang";
    };

    QString authEndpoint()
    {
        return "https://authserver.mojang.com/";
    };

    bool canChangeSkin()
    {
        return true;
    };
};
