/* Copyright 2013-2019 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fstream>
#include <string>

#include "PrintInstanceInfo.h"
#include <launch/LaunchTask.h>

#ifdef Q_OS_LINUX
namespace {
void probeProcCpuinfo(QStringList &log)
{
    std::ifstream cpuin("/proc/cpuinfo");
    for (std::string line; std::getline(cpuin, line);)
    {
        if (strncmp(line.c_str(), "model name", 10) == 0)
        {
            log << QString::fromStdString(line.substr(13, std::string::npos));
            break;
        }
    }
}

void runLspci(QStringList &log)
{
    // FIXME: fixed size buffers...
    char buff[512];
    int gpuline = -1;
    int cline = 0;
    FILE * lspci = popen("lspci -k", "r");

    if (!lspci)
        return;

    while (fgets(buff, 512, lspci) != NULL)
    {
        std::string str(buff);
        if (str.length() < 9)
            continue;
        if (str.substr(8, 3) == "VGA")
        {
            gpuline = cline;
            log << QString::fromStdString(str.substr(35, std::string::npos));
        }
        if (gpuline > -1 && gpuline != cline)
        {
            if (cline - gpuline < 3)
            {
                log << QString::fromStdString(str.substr(1, std::string::npos));
            }
        }
        cline++;
    }
    pclose(lspci);
}

void runGlxinfo(QStringList & log)
{
    // FIXME: fixed size buffers...
    char buff[512];
    FILE *glxinfo = popen("glxinfo", "r");
    if (!glxinfo)
        return;

    while (fgets(buff, 512, glxinfo) != NULL)
    {
        if (strncmp(buff, "OpenGL version string:", 22) == 0)
        {
            log << QString::fromUtf8(buff);
            break;
        }
    }
    pclose(glxinfo);
}

}
#endif

#ifdef Q_OS_WIN32
#include <windows.h>

namespace {
void printDisplayFlags(QStringList & log, const DWORD flags, const char * spacing)
{
    QStringList flagStrings;
    if(flags & DISPLAY_DEVICE_ACTIVE)
    {
        flags << "Active";
    }
    if(flags & DISPLAY_DEVICE_PRIMARY_DEVICE)
    {
        flags << "Primary";
    }
    if(flags & DISPLAY_DEVICE_MIRRORING_DRIVER)
    {
        flags << "Mirroring";
    }
    if(flagStrings.size())
    {
        log << QString("%1%2").arg(spacing, flagStrings.join(','));
    }
}

void probeWinAPIForDevices(QStringList & log)
{
    DISPLAY_DEVICEW dd;
    memset(&dd, 0, sizeof(DISPLAY_DEVICEW));
    int i = 0;

    // enumerate devices
    while(EnumDisplayDevicesW(NULL, i, &dd, 0))
    {
        log << "Display devices:";
        log << QString("Device Name: %1 | Device String: %2").arg(QString::fromWCharArray(dd.DeviceName), QString::fromWCharArray(dd.DeviceString));
        printDisplayFlags(log, dd.StateFlags, "    ");

        // enumerate monitors
        if(EnumDisplayDevicesW(dd.DeviceName, 0, &dd, 0))
        {
            log << QString("    Monitor Name: %1 | Monitor String: %2").arg(QString::fromWCharArray(dd.DeviceName), QString::fromWCharArray(dd.DeviceString));
            printDisplayFlags(log, dd.StateFlags, "        ");
        }
        i++;
    }
}
}
#endif

void PrintInstanceInfo::executeTask()
{
    auto instance = m_parent->instance();
    QStringList log;

#ifdef Q_OS_LINUX
    ::probeProcCpuinfo(log);
    ::runLspci(log);
    ::runGlxinfo(log);
#endif

#ifdef Q_OS_WIN32
    ::probeWinAPIForDevices(log);
#endif

    logLines(log, MessageLevel::MultiMC);
    logLines(instance->verboseDescription(m_session), MessageLevel::MultiMC);
    emitSucceeded();
}
