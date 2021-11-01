#pragma once

#include <QObject>
#include "QObjectPtr.h"
#include <QDateTime>
#include <QSet>
#include <QProcess>
#include <QDebug>

#include "BaseAuthProvider.h"

class ElybyAuthProvider : public BaseAuthProvider
{
    Q_OBJECT

public:
    QString id()
    {
        return "elyby";
    }

    QString displayName()
    {
        return "Ely.by";
    };

    QString injectorEndpoint()
    {
        return "ely.by";
    };

    QString authEndpoint()
    {
        return "https://authserver.ely.by/auth/";
    };

    QUrl resolveSkinUrl(QString id, QString name)
    {
        return QUrl(((QString) "http://skinsystem.ely.by/skins/%1.png").arg(name));
    }

    virtual bool useYggdrasil()
    {
        return true;
    }
};
