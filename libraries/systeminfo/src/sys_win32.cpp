#include "sys.h"

#include <windows.h>
#include <QDebug>

#include "ntstatus/NtStatusNames.hpp"

#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 12
#endif

Sys::KernelInfo Sys::getKernelInfo()
{
    Sys::KernelInfo out;
    out.kernelType = KernelType::Windows;
    out.kernelName = "Windows";
    OSVERSIONINFOW osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOW));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
    GetVersionExW(&osvi);
    out.kernelVersion = QString("%1.%2").arg(osvi.dwMajorVersion).arg(osvi.dwMinorVersion);
    out.kernelMajor = osvi.dwMajorVersion;
    out.kernelMinor = osvi.dwMinorVersion;
    out.kernelPatch = osvi.dwBuildNumber;
    return out;
}

uint64_t Sys::getSystemRam()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx( &status );
    // bytes
    return (uint64_t)status.ullTotalPhys;
}

bool Sys::isSystem64bit()
{
#if defined(_WIN64)
    return true;
#elif defined(_WIN32)
    BOOL f64 = false;
    return IsWow64Process(GetCurrentProcess(), &f64) && f64;
#else
    // it's some other kind of system...
    return false;
#endif
}

bool Sys::isCPU64bit()
{
    SYSTEM_INFO info;
    ZeroMemory(&info, sizeof(SYSTEM_INFO));
    GetNativeSystemInfo(&info);
    auto arch = info.wProcessorArchitecture;
    return arch == PROCESSOR_ARCHITECTURE_AMD64 || arch == PROCESSOR_ARCHITECTURE_IA64;
}

Sys::DistributionInfo Sys::getDistributionInfo()
{
    DistributionInfo result;
    return result;
}

bool Sys::lookupSystemStatusCode(uint64_t code, std::string &name, std::string &description)
{
    bool hasCodeName = NtStatus::lookupNtStatusCodeName(code, name);

    PSTR messageBuffer = nullptr;
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if(!ntdll)
    {
        // ???
        qWarning() << "GetModuleHandleA returned nullptr for ntdll.dll";
        return false;
    }

    auto messageSize = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
            ntdll,
            code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<PSTR>(&messageBuffer),
            0,
            nullptr
    );

    bool hasDescription = messageSize > 0;
    if(hasDescription)
    {
        description = std::string(messageBuffer, messageSize);
    }

    if(messageBuffer)
    {
        LocalFree(messageBuffer);
    }

    return hasCodeName || hasDescription;
}

Sys::Architecture Sys::systemArchitecture() {
    SYSTEM_INFO info;
    ZeroMemory(&info, sizeof(SYSTEM_INFO));
    GetNativeSystemInfo(&info);
    auto arch = info.wProcessorArchitecture;

    QString qtArch = QSysInfo::currentCpuArchitecture();
    switch (arch) {
        case PROCESSOR_ARCHITECTURE_AMD64:
            return { ArchitectureType::AMD64, "x86_64" };
        case PROCESSOR_ARCHITECTURE_ARM64:
            return { ArchitectureType::ARM64, "arm64" };
        case PROCESSOR_ARCHITECTURE_INTEL:
            return { ArchitectureType::I386, "i386" };
        default:
            return { ArchitectureType::Undetermined, qtArch };
    }
}
