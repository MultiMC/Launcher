#include "CheckHistory.h"

namespace UpdaterV2
{

bool CheckHistory::record(bool verified, qint64 now)
{
    if(verified)
    {
        lastSuccess = now;
        failingSince = 0;
        failedChecks = 0;
        return false;
    }
    // a clock that went back starts the wait over, rather than ending it early or never
    if(failingSince <= 0 || failingSince > now)
    {
        failingSince = now;
        failedChecks = 0;
    }
    failedChecks++;
    if(failedChecks < FailingChecks || now - failingSince < FailingSeconds)
    {
        return false;
    }
    failingSince = now;
    failedChecks = 0;
    return true;
}

}
