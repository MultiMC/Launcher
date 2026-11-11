#pragma once
#include <QtGlobal>

namespace UpdaterV2
{

/**
 * How the update checks have been going, kept in the settings: enough to notice that they
 * keep failing. A check that fails once is nothing; checks that fail for weeks mean this
 * installation may never update again, whatever the reason (servers that moved, keys that
 * expired, a mistake in what was published, a mistake in this build), and its user is the
 * only one who can do something about it. So they are told, which a failed automatic check
 * otherwise never does.
 */
struct CheckHistory
{
    // checks have to fail for this long, and this many times, before the user is told
    static const qint64 FailingSeconds = 14 * 24 * 60 * 60;
    static const int FailingChecks = 3;

    // when a check last got a verified answer, in seconds since 1970; 0: never
    qint64 lastSuccess = 0;
    // when the first check since then failed; 0: checks aren't failing
    qint64 failingSince = 0;
    // how many have failed since then
    int failedChecks = 0;

    /**
     * Record the outcome of a check at `now`: `verified` when it got a verified answer,
     * whatever the answer was. True when checks have been failing for long enough to tell the
     * user; the count then starts over, so they are told again if it goes on.
     */
    bool record(bool verified, qint64 now);
};

}
