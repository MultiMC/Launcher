#include "UpdateStep.h"

namespace Meta
{
namespace
{
bool within(qint64 ago, qint64 limit)
{
    return ago >= 0 && ago < limit;
}
}

UpdateStep nextUpdateStep(const UpdateState &state)
{
    using Fetch = UpdateState::Fetch;
    if (state.fetch == Fetch::NotYet)
    {
        if (state.isRoot)
        {
            if (within(state.rootFailedAgo, RootRetryDelay))
            {
                return UpdateStep::Done;
            }
            if (!state.forced && within(state.rootFetchedAgo, RootLifetime))
            {
                return UpdateStep::Done;
            }
            return UpdateStep::Fetch;
        }
        if (state.parentBusy)
        {
            return UpdateStep::WaitForParent;
        }
        if (state.current)
        {
            return UpdateStep::Done;
        }
        return UpdateStep::Fetch;
    }

    // fetching it failed
    if (!state.isRoot && state.fetch == Fetch::Refused && !state.rootRefreshed)
    {
        // a fresh root index, or one that can't be fetched now, won't say anything new
        bool rootHelps = state.rootBusy
            || (!within(state.rootFetchedAgo, RootMinimumAge) && !within(state.rootFailedAgo, RootRetryDelay));
        if (rootHelps)
        {
            return UpdateStep::RefreshRoot;
        }
    }
    if (state.hasLocal)
    {
        return UpdateStep::UseLocal;
    }
    return UpdateStep::Fail;
}
}
