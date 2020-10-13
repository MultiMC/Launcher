#pragma once

#include "BaseVersion.h"
#include "JavaVersion.h"
#include <sys.h>

struct JavaInstall : public BaseVersion
{
    JavaInstall(){}
    JavaInstall(const QString& id, const Sys::Architecture& arch, const QString& path)
    : id(id), arch(arch), path(path)
    {
    }
    virtual QString descriptor()
    {
        return id.toString();
    }

    virtual QString name()
    {
        return id.toString();
    }

    virtual QString typeString() const
    {
        return arch.serialize();
    }

    bool operator<(const JavaInstall & rhs);
    bool operator==(const JavaInstall & rhs);
    bool operator>(const JavaInstall & rhs);

    JavaVersion id;
    Sys::Architecture arch;
    QString path;
    bool recommended = false;

    bool remote = false;
    QString url;
    QString installRoot;
};

typedef std::shared_ptr<JavaInstall> JavaInstallPtr;
