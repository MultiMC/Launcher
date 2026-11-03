#include "Mirrors.h"

#include <QDateTime>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>

#include <algorithm>
#include <random>

#include "BuildConfig.h"

namespace {
// how long a base URL that couldn't be reached is tried last
const qint64 unreachableMsecs = 5 * 60 * 1000;

void randomShuffle(QStringList &list)
{
    // MinGW's std::random_device can give the same numbers every time, so the time goes in too
    std::random_device device;
    std::mt19937 generator(device() ^ uint(QDateTime::currentMSecsSinceEpoch()));
    std::shuffle(list.begin(), list.end(), generator);
}

struct State
{
    QMutex mutex;
    QList<QStringList> sites{BuildConfig.FILES_MIRRORS, BuildConfig.META_MIRRORS};
    // site (its first base URL) -> its mirror list
    QMap<QString, QList<QStringList>> mirrorLists;
    // base URL -> until when it is tried last
    QMap<QString, qint64> unreachable;
    std::function<void(QStringList &)> shuffle = randomShuffle;
};

State &state()
{
    static State s;
    return s;
}

// every base URL of a site, its mirror list's first, in the order to try them
QStringList baseUrls(const QStringList &site)
{
    QStringList out;
    for(auto group: state().mirrorLists.value(site.first()))
    {
        state().shuffle(group);
        out.append(group);
    }
    for(const auto &base: site)
    {
        if(!out.contains(base))
        {
            out.append(base);
        }
    }
    return out;
}

// the base URL url starts with, and the site it belongs to; an empty site if it is on none
QStringList siteOf(const QString &url, QString &baseUrl)
{
    for(const auto &site: state().sites)
    {
        QStringList bases = site;
        for(const auto &group: state().mirrorLists.value(site.first()))
        {
            bases.append(group);
        }
        for(const auto &base: bases)
        {
            if(url.startsWith(base))
            {
                baseUrl = base;
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
    state().mirrorLists.clear();
    state().unreachable.clear();
}

void setMirrorList(const QString &site, const QList<QStringList> &groups)
{
    QMutexLocker locker(&state().mutex);
    if(groups.isEmpty())
    {
        state().mirrorLists.remove(site);
    }
    else
    {
        state().mirrorLists.insert(site, groups);
    }
}

void setShuffle(std::function<void(QStringList &)> shuffle)
{
    QMutexLocker locker(&state().mutex);
    state().shuffle = shuffle ? shuffle : randomShuffle;
}

QList<QUrl> candidates(const QUrl &url)
{
    QMutexLocker locker(&state().mutex);
    const QString text = url.toString(QUrl::FullyEncoded);
    QString baseUrl;
    const auto site = siteOf(text, baseUrl);
    if(site.isEmpty())
    {
        return {url};
    }
    const QString path = text.mid(baseUrl.size());
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QList<QUrl> reachable, unreachable;
    for(const auto &base: baseUrls(site))
    {
        auto &list = state().unreachable.value(base) > now ? unreachable : reachable;
        list.append(QUrl(base + path));
    }
    return reachable + unreachable;
}

void markUnreachable(const QUrl &url)
{
    QMutexLocker locker(&state().mutex);
    QString baseUrl;
    if(!siteOf(url.toString(QUrl::FullyEncoded), baseUrl).isEmpty())
    {
        state().unreachable[baseUrl] = QDateTime::currentMSecsSinceEpoch() + unreachableMsecs;
    }
}

}
}
