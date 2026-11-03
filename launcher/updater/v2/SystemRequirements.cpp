#include "SystemRequirements.h"

#include <QObject>
#include <QSysInfo>
#include <QVector>

#include <functional>

#include <sys.h>

#if defined(Q_OS_LINUX) && defined(__GLIBC__)
#include <gnu/libc-version.h>
#include <link.h>
#include <cstring>
#endif

#if defined(Q_OS_MAC)
#include <sys/sysctl.h>
#endif

namespace {

bool parseVersion(const QString &version, QVector<uint> &parts)
{
    parts.clear();
    if(version.isEmpty())
    {
        return false;
    }
    for(auto &part: version.split('.'))
    {
        // at most 9 digits, so it fits
        if(part.isEmpty() || part.size() > 9 || (part.size() > 1 && part[0] == '0'))
        {
            return false;
        }
        for(auto c: part)
        {
            if(c < '0' || c > '9')
            {
                return false;
            }
        }
        parts.append(part.toUInt());
    }
    return true;
}

QString archName(const Sys::Architecture &arch)
{
    if(arch == Sys::Architecture(Sys::ArchitectureType::X86))
    {
        return "x86";
    }
    if(arch == Sys::Architecture(Sys::ArchitectureType::AMD64))
    {
        return "x86_64";
    }
    if(arch == Sys::Architecture(Sys::ArchitectureType::AARCH64))
    {
        return "arm64";
    }
    return arch.getRaw();
}

QString osDisplayName(const QString &os)
{
    if(os == "windows")
    {
        return "Windows";
    }
    if(os == "macos")
    {
        return "macOS";
    }
    if(os == "linux")
    {
        return "Linux";
    }
    return os;
}

QString windowsDisplayName(const QString &ntVersion)
{
    QVector<uint> parts;
    if(!parseVersion(ntVersion, parts))
    {
        return QString("Windows NT %1").arg(ntVersion);
    }
    parts.resize(2);
    QString name;
    if(parts[0] == 5 && parts[1] == 1)
    {
        name = "Windows XP";
    }
    else if(parts[0] == 5 && parts[1] == 2)
    {
        name = "Windows XP x64";
    }
    else if(parts[0] == 6 && parts[1] == 0)
    {
        name = "Windows Vista";
    }
    else if(parts[0] == 6 && parts[1] == 1)
    {
        name = "Windows 7";
    }
    else if(parts[0] == 6 && parts[1] == 2)
    {
        name = "Windows 8";
    }
    else if(parts[0] == 6 && parts[1] == 3)
    {
        name = "Windows 8.1";
    }
    else if(parts[0] == 10 && parts[1] == 0)
    {
        name = "Windows 10";
    }
    else
    {
        return QString("Windows NT %1").arg(ntVersion);
    }
    return QString("%1 (NT %2)").arg(name, ntVersion);
}

QString osVersionDisplayName(const QString &os, const QString &version)
{
    if(os == "windows")
    {
        return windowsDisplayName(version);
    }
    return QString("%1 %2").arg(osDisplayName(os), version);
}

// Whether a machine of this architecture runs builds for that one
bool archRuns(const QString &os, const QString &machine, const QString &build)
{
    if(machine == build)
    {
        return true;
    }
    if(os == "windows" && machine == "x86_64" && build == "x86")
    {
        return true;
    }
    // Rosetta
    if(os == "macos" && machine == "arm64" && build == "x86_64")
    {
        return true;
    }
    return false;
}

#if defined(Q_OS_LINUX) && defined(__GLIBC__)
/*
 * Finds the highest GLIBCXX_ version the loaded libstdc++ defines, from the version
 * definitions in its dynamic section. libstdc++ has no call that says this.
 */
int findGlibcxxVersion(struct dl_phdr_info *info, size_t, void *data)
{
    auto result = static_cast<QString *>(data);
    const char *slash = std::strrchr(info->dlpi_name, '/');
    const char *fileName = slash ? slash + 1 : info->dlpi_name;
    if(std::strncmp(fileName, "libstdc++.so", 12) != 0)
    {
        return 0;
    }

    const ElfW(Dyn) *dynamic = nullptr;
    for(int i = 0; i < info->dlpi_phnum; i++)
    {
        if(info->dlpi_phdr[i].p_type == PT_DYNAMIC)
        {
            dynamic = reinterpret_cast<const ElfW(Dyn) *>(info->dlpi_addr + info->dlpi_phdr[i].p_vaddr);
        }
    }
    if(!dynamic)
    {
        return 1;
    }
    ElfW(Addr) strtab = 0;
    ElfW(Addr) verdef = 0;
    ElfW(Xword) verdefCount = 0;
    for(auto entry = dynamic; entry->d_tag != DT_NULL; entry++)
    {
        switch(entry->d_tag)
        {
            case DT_STRTAB:
                strtab = entry->d_un.d_ptr;
                break;
            case DT_VERDEF:
                verdef = entry->d_un.d_ptr;
                break;
            case DT_VERDEFNUM:
                verdefCount = entry->d_un.d_val;
                break;
        }
    }
    if(!strtab || !verdef)
    {
        return 1;
    }
    // The dynamic linker relocates these in place on most architectures, but not on all
    if(strtab < info->dlpi_addr)
    {
        strtab += info->dlpi_addr;
    }
    if(verdef < info->dlpi_addr)
    {
        verdef += info->dlpi_addr;
    }

    auto definition = reinterpret_cast<const ElfW(Verdef) *>(verdef);
    for(ElfW(Xword) i = 0; i < verdefCount; i++)
    {
        auto aux = reinterpret_cast<const ElfW(Verdaux) *>(reinterpret_cast<const char *>(definition) + definition->vd_aux);
        QString name = QString::fromLatin1(reinterpret_cast<const char *>(strtab + aux->vda_name));
        if(name.startsWith("GLIBCXX_"))
        {
            auto version = name.mid(8);
            if(SystemRequirements::isValidVersion(version) && (result->isEmpty() || SystemRequirements::compareVersions(version, *result) > 0))
            {
                *result = version;
            }
        }
        if(!definition->vd_next)
        {
            break;
        }
        definition = reinterpret_cast<const ElfW(Verdef) *>(reinterpret_cast<const char *>(definition) + definition->vd_next);
    }
    return 1;
}
#endif

}

bool SystemRequirements::isValidVersion(const QString &version)
{
    QVector<uint> parts;
    return parseVersion(version, parts);
}

int SystemRequirements::compareVersions(const QString &a, const QString &b)
{
    QVector<uint> partsA;
    QVector<uint> partsB;
    parseVersion(a, partsA);
    parseVersion(b, partsB);
    int size = qMax(partsA.size(), partsB.size());
    partsA.resize(size);
    partsB.resize(size);
    for(int i = 0; i < size; i++)
    {
        if(partsA[i] != partsB[i])
        {
            return partsA[i] < partsB[i] ? -1 : 1;
        }
    }
    return 0;
}

SystemRequirements::System SystemRequirements::detect()
{
    System out;
    auto kernel = Sys::getKernelInfo();
#if defined(Q_OS_WIN)
    out.os = "windows";
    // GetVersionEx, truthful since the manifest declares the Windows versions we support
    out.osVersion = QString("%1.%2.%3").arg(kernel.kernelMajor).arg(kernel.kernelMinor).arg(kernel.kernelPatch);
    out.machineArch = archName(Sys::systemArchitecture());
#elif defined(Q_OS_MAC)
    out.os = "macos";
    out.osVersion = QSysInfo::productVersion();
    out.machineArch = archName(Sys::systemArchitecture());
    // under Rosetta, uname says x86_64
    int translated = 0;
    size_t size = sizeof(translated);
    if(sysctlbyname("sysctl.proc_translated", &translated, &size, nullptr, 0) == 0 && translated == 1)
    {
        out.machineArch = "arm64";
    }
#else
#if defined(Q_OS_LINUX)
    out.os = "linux";
#else
    out.os = QSysInfo::kernelType();
#endif
    out.osVersion = QString("%1.%2.%3").arg(kernel.kernelMajor).arg(kernel.kernelMinor).arg(kernel.kernelPatch);
    out.machineArch = archName(Sys::systemArchitecture());
#endif
    out.launcherArch = archName(Sys::Architecture::fromOSArch(QSysInfo::buildCpuArchitecture()));
    if(!isValidVersion(out.osVersion) || out.osVersion == "0.0.0")
    {
        out.osVersion.clear();
    }

#if defined(Q_OS_LINUX) && defined(__GLIBC__)
    out.glibc = QString::fromLatin1(gnu_get_libc_version());
    if(!isValidVersion(out.glibc))
    {
        out.glibc.clear();
    }
    dl_iterate_phdr(findGlibcxxVersion, &out.glibcxx);
#endif

    out.qt = QString::fromLatin1(qVersion());
    return out;
}

QString SystemRequirements::requirementArch(const System &system)
{
    if(system.os == "linux")
    {
        return system.launcherArch;
    }
    return system.machineArch;
}

QStringList SystemRequirements::unmetRequirements(const Requirements &requirements, const System &system)
{
    QStringList out;
    const auto &os = requirements.os;
    auto incomplete = [&](const QString &what)
    {
        out.append(QObject::tr("The build's requirements are incomplete or not understood (%1).").arg(what));
    };

    if(os != "windows" && os != "macos" && os != "linux")
    {
        incomplete(QString("os %1").arg(os));
        return out;
    }
    if(system.os != os)
    {
        out.append(QObject::tr("The build is for %1, this computer runs %2.").arg(osDisplayName(os), osDisplayName(system.os)));
        return out;
    }

    if(requirements.arch.isEmpty())
    {
        incomplete("arch");
    }
    else if(!archRuns(os, requirementArch(system), requirements.arch))
    {
        auto arch = requirementArch(system);
        out.append(QObject::tr("The build needs a %1 processor, this computer has %2.").arg(requirements.arch, arch.isEmpty() ? QObject::tr("an unknown one") : arch));
    }

    // a minimum version: `what` names the requirement, `describe` makes a version readable
    auto checkMinimum = [&](const QString &what, const QString &required, const QString &found, std::function<QString(const QString &)> describe)
    {
        if(!isValidVersion(required))
        {
            incomplete(QString("%1 %2").arg(what, required));
        }
        else if(!isValidVersion(found))
        {
            out.append(QObject::tr("The build needs %1 or newer, and the version on this computer could not be found out.").arg(describe(required)));
        }
        else if(compareVersions(found, required) < 0)
        {
            out.append(QObject::tr("The build needs %1 or newer, this computer has %2.").arg(describe(required), describe(found)));
        }
    };

    if(os == "windows" || os == "macos")
    {
        checkMinimum("os_version", requirements.osVersion, system.osVersion, [&](const QString &version)
        {
            return osVersionDisplayName(os, version);
        });
    }
    else
    {
        checkMinimum("glibc", requirements.glibc, system.glibc, [](const QString &version)
        {
            return QString("glibc %1").arg(version);
        });
        checkMinimum("glibcxx", requirements.glibcxx, system.glibcxx, [](const QString &version)
        {
            return QString("libstdc++ GLIBCXX_%1").arg(version);
        });

        QVector<uint> required;
        QVector<uint> found;
        if(!parseVersion(requirements.qt, required))
        {
            incomplete(QString("qt %1").arg(requirements.qt));
        }
        else if(!parseVersion(system.qt, found))
        {
            out.append(QObject::tr("The build needs Qt %1 or a newer Qt %2, and the version on this computer could not be found out.").arg(requirements.qt).arg(required[0]));
        }
        else if(found[0] != required[0] || compareVersions(system.qt, requirements.qt) < 0)
        {
            out.append(QObject::tr("The build needs Qt %1 or a newer Qt %2, this computer has Qt %3.").arg(requirements.qt).arg(required[0]).arg(system.qt));
        }
    }
    return out;
}
