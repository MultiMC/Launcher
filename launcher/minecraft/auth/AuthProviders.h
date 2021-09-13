#pragma once

#include <QObject>
#include "QObjectPtr.h"
#include <QDateTime>
#include <QSet>
#include <QProcess>
#include <QMap>

#include "providers/BaseAuthProvider.h"

/*!
 * \brief Namespace for auth providers.
 * This class main putpose is to handle registration and lookup of auth providers
 */
namespace AuthProviders
{
    void load();
    AuthProviderPtr lookup(QString id);
    QList<AuthProviderPtr> getAll();
}
