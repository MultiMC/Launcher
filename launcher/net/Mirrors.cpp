#include "Mirrors.h"

#include <QDateTime>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>

#include "BuildConfig.h"

namespace {
// how long a base URL that couldn't be reached is tried last
const qint64 unreachableMsecs = 5 * 60 * 1000;

struct State
{
    QMutex mutex;
    QList<QStringList> sites{BuildConfig.FILES_MIRRORS, BuildConfig.META_MIRRORS};
    // base URL -> until when it is tried last
    QMap<QString, qint64> unreachable;
};

State &state()
{
    static State s;
    return s;
}

// the site url is on, and the length of its base URL; an empty site if it is on none
QStringList siteOf(const QString &url, int &baseLength)
{
    for(const auto &site: state().sites)
    {
        for(const auto &base: site)
        {
            if(url.startsWith(base))
            {
                baseLength = base.size();
                return site;
            }
        }
    }
    return {};
}
}

namespace Net {
namespace Mirrors {

void setSites(const QList<QStringList> &sites)
{
    QMutexLocker locker(&state().mutex);
    state().sites = sites;
    state().unreachable.clear();
}

QList<QUrl> candidates(const QUrl &url)
{
    QMutexLocker locker(&state().mutex);
    const QString text = url.toString(QUrl::FullyEncoded);
    int baseLength = 0;
    const auto site = siteOf(text, baseLength);
    if(site.isEmpty())
    {
        return {url};
    }
    const QString path = text.mid(baseLength);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QList<QUrl> reachable, unreachable;
    for(const auto &base: site)
    {
        auto &list = state().unreachable.value(base) > now ? unreachable : reachable;
        list.append(QUrl(base + path));
    }
    return reachable + unreachable;
}

void markUnreachable(const QUrl &url)
{
    QMutexLocker locker(&state().mutex);
    const QString text = url.toString(QUrl::FullyEncoded);
    for(const auto &site: state().sites)
    {
        for(const auto &base: site)
        {
            if(text.startsWith(base))
            {
                state().unreachable[base] = QDateTime::currentMSecsSinceEpoch() + unreachableMsecs;
                return;
            }
        }
    }
}

}
}
