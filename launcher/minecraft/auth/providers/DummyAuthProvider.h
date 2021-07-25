#pragma once

#include <QObject>
#include "QObjectPtr.h"
#include <QDateTime>
#include <QSet>
#include <QProcess>
#include <QDebug>

#include "BaseAuthProvider.h"

class DummyAuthProvider : public BaseAuthProvider
{
    Q_OBJECT

public:
    QString id()
    {
        return "dummy";
    }

    QString displayName()
    {
        return "Local";
    }

    bool dummyAuth()
    {
        return true;
    }

    QString injectorEndpoint()
    {
        return "http://localhost:%1";
    };
};
