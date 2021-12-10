#pragma once

#include <QObject>
#include "QObjectPtr.h"
#include <QDateTime>
#include <QSet>
#include <QProcess>
#include <QDebug>

#include "MojangAuthProvider.h"

class MicrosoftAuthProvider : public MojangAuthProvider
{
    Q_OBJECT

public:
    QString id()
    {
        return "MSA";
    }

    QString displayName()
    {
        return "Microsoft";
    }
};
