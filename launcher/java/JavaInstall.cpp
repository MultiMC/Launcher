#include "JavaInstall.h"
#include <MMCStrings.h>

bool JavaInstall::operator<(const JavaInstall &rhs)
{
    // prefer remote
    if(remote < rhs.remote) {
        return true;
    }
    if(remote > rhs.remote) {
        return false;
    }
    // FIXME: make this prefer native arch
    auto archCompare = Strings::naturalCompare(arch, rhs.arch, Qt::CaseInsensitive);
    if(archCompare != 0)
        return archCompare < 0;

    // FIXME: make this a version compare
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
