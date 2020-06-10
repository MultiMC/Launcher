#include "PackInstallTask.h"

namespace ModpacksCH {

PackInstallTask::PackInstallTask(Modpack pack, QString version)
{
    m_pack = pack;
    m_version = version;
}

bool PackInstallTask::abort()
{
    return true;
}

void PackInstallTask::executeTask()
{
}

}
