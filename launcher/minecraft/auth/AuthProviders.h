#pragma once

#include <QObject>
#include "QObjectPtr.h"
#include <QDateTime>
#include <QSet>
#include <QProcess>
#include <QMap>

#include "providers/BaseAuthProvider.h"
#include "../../AuthServer.h"

/*!
 * \brief Namespace for auth providers.
 * This class main putpose is to handle registration and lookup of auth providers
 */
namespace AuthProviders
{
    void load(std::shared_ptr<AuthServer> authServer);
    AuthProviderPtr lookup(QString id);
    QList<AuthProviderPtr> getAll();
}
