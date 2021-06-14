#pragma once

#include <QObject>
#include "QObjectPtr.h"
#include <QDateTime>
#include <QSet>
#include <QProcess>
#include <QMap>

#include "providers/BaseAuthProvider.h"
#include "multimc_logic_export.h"

/*!
 * \brief Namespace for auth providers.
 * This class main putpose is to handle registration and lookup of auth providers
 */
namespace AuthProviders
{
  MULTIMC_LOGIC_EXPORT void load();
  MULTIMC_LOGIC_EXPORT AuthProviderPtr lookup(QString id);
  MULTIMC_LOGIC_EXPORT QList<AuthProviderPtr> getAll();
}
