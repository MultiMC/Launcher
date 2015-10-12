#include "WonkoUtil.h"

#include <QUrl>
#include <QDir>

#include "Env.h"

namespace Wonko
{
QUrl rootUrl()
{
	return ENV.wonkoRootUrl();
}
QUrl indexUrl()
{
	return rootUrl().resolved(QStringLiteral("index.json"));
}
QUrl versionListUrl(const QString &uid)
{
	return rootUrl().resolved(uid + ".json");
}
QUrl versionUrl(const QString &uid, const QString &version)
{
	return rootUrl().resolved(uid + "/" + version + ".json");
}

QDir localWonkoDir()
{
	return QDir("wonko");
}

}
