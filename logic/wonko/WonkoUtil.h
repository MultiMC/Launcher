#pragma once

#include "multimc_logic_export.h"

class QUrl;
class QString;
class QDir;

namespace Wonko
{
MULTIMC_LOGIC_EXPORT QUrl rootUrl();
MULTIMC_LOGIC_EXPORT QUrl indexUrl();
MULTIMC_LOGIC_EXPORT QUrl versionListUrl(const QString &uid);
MULTIMC_LOGIC_EXPORT QUrl versionUrl(const QString &uid, const QString &version);
MULTIMC_LOGIC_EXPORT QDir localWonkoDir();
}
