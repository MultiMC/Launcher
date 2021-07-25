#pragma once

#include <QObject>
#include "QObjectPtr.h"
#include <QDateTime>
#include <QSet>
#include <QProcess>
#include <QMap>

#include "providers/BaseAuthProvider.h"
#include "logic_export.h"

/*!
 * \brief Namespace for auth providers.
 * This class main putpose is to handle registration and lookup of auth providers
 */
namespace AuthProviders
{
    LOGIC_EXPORT void load();
    LOGIC_EXPORT AuthProviderPtr lookup(QString id);
    LOGIC_EXPORT QList<AuthProviderPtr> getAll();
}
