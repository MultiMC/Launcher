#pragma once

#include <QtGlobal>

namespace Meta
{
/// meta refreshes once an hour, so the root index is fetched again when it is older than this
const qint64 RootLifetime = 55 * 60 * 1000;
/// a root index fetched more recently than this can't explain a hash that doesn't match
const qint64 RootMinimumAge = 60 * 1000;
/// after failing to fetch the root index, wait this long before trying again
const qint64 RootRetryDelay = 5 * 60 * 1000;

/// What updating a meta entity does next
enum class UpdateStep
{
    /// nothing to do: it's current, or can't be fetched now
    Done,
    /// the parent is being updated, and says what is current
    WaitForParent,
    /// download it
    Fetch,
    /// fetch the root index again, then follow the path down once more
    RefreshRoot,
    /// it couldn't be updated: use the local copy as it is, stale isn't invalid
    UseLocal,
    /// it couldn't be updated, and there is nothing local to use
    Fail
};

/// Everything the next step depends on
struct UpdateState
{
    enum class Fetch
    {
        /// not tried yet
        NotYet,
        /// the content was refused (a hash that didn't match), or isn't there: the path from the root is stale
        Refused,
        /// anything else: unreachable, a server error
        Failed
    };

    /// this is the root index
    bool isRoot = false;
    /// how long ago the root index was fetched, and fetching it last failed; -1 for never
    qint64 rootFetchedAgo = -1;
    qint64 rootFailedAgo = -1;
    /// the root index is being fetched right now
    bool rootBusy = false;
    /// the root index is fetched again because a hash didn't match, whatever its age
    bool forced = false;

    /// the parent is being updated
    bool parentBusy = false;
    /// the local copy is what the parent lists
    bool current = false;
    /// there is a local copy
    bool hasLocal = false;

    /// how fetching it went
    Fetch fetch = Fetch::NotYet;
    /// this update already fetched the root index again once
    bool rootRefreshed = false;
};

UpdateStep nextUpdateStep(const UpdateState &state);
}
