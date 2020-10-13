#include "JavaInstall.h"
#include <MMCStrings.h>

bool JavaInstall::operator<(const JavaInstall &rhs)
{
    if(arch < rhs.arch)
    {
        return true;
    }
    if(arch > rhs.arch)
    {
        return false;
    }
    if(id < rhs.id)
    {
        return true;
    }
    if(id > rhs.id)
    {
        return false;
    }
    return Strings::naturalCompare(path, rhs.path, Qt::CaseInsensitive) < 0;
}

bool JavaInstall::operator==(const JavaInstall &rhs)
{
    return arch == rhs.arch && id == rhs.id && path == rhs.path && remote == rhs.remote;
}

bool JavaInstall::operator>(const JavaInstall &rhs)
{
    return (!operator<(rhs)) && (!operator==(rhs));
}
