#pragma once

#include <QList>
#include <QStringList>
#include <QUrl>

#include <functional>

namespace Net {
/*
 * Our sites and their copies (the Design repo's LauncherNetwork.md). A download from one of a
 * site's base URLs falls back to the same path on the site's other base URLs, in order.
 */
namespace Mirrors {
/// The sites, each a list of base URLs with the main server first. By default the ones in
/// BuildConfig. Setting them forgets which base URLs couldn't be reached.
void setSites(const QList<QStringList> &sites);

/// A mirror list for the site whose first base URL is site (a signed one, like the new
/// updater's): groups of base URLs, tried in order, the base URLs within a group in random
/// order, then the site's own base URLs that aren't in the list. An empty list goes back to
/// the site's own base URLs. Ignored when there is no such site.
void setMirrorList(const QString &site, const QList<QStringList> &groups);

/// The URLs to try for url, in order: its path on each base URL of its site, the main server
/// first (or the site's mirror list), and the base URLs that couldn't be reached in the last
/// few minutes last. Just url when it isn't on one of the sites.
QList<QUrl> candidates(const QUrl &url);

/// For tests: how a group of the mirror list is put in random order
void setShuffle(std::function<void(QStringList &)> shuffle);

/// Remembers for a few minutes that the base URL of url couldn't be reached
void markUnreachable(const QUrl &url);
}
}
