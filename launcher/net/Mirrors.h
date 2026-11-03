#pragma once

#include <QList>
#include <QStringList>
#include <QUrl>

namespace Net {
/*
 * Our sites and their copies (the Design repo's LauncherNetwork.md). A download from one of a
 * site's base URLs falls back to the same path on the site's other base URLs, in order.
 */
namespace Mirrors {
/// The sites, each a list of base URLs with the main server first. By default the ones in
/// BuildConfig. Setting them forgets which base URLs couldn't be reached.
void setSites(const QList<QStringList> &sites);

/// The URLs to try for url, in order: its path on each base URL of its site, the main server
/// first, and the base URLs that couldn't be reached in the last few minutes last.
/// Just url when it isn't on one of the sites.
QList<QUrl> candidates(const QUrl &url);

/// Remembers for a few minutes that the base URL of url couldn't be reached
void markUnreachable(const QUrl &url);
}
}
