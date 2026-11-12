#pragma once
#include <QString>
#include <QStringList>

/**
 * The system a build runs on, and what a build requires of it.
 *
 * Names and values are those of the update manifests' `requires` (see the Requirements
 * section of Design/UpdaterFormats.md in the MultiMC repository): operating systems
 * `windows`, `macos` and `linux`, architectures `x86`, `x86_64` and `arm64`, versions as
 * numbers separated by dots.
 */
namespace SystemRequirements
{

/**
 * What the Launcher found about the system it runs on. Values that couldn't be found are empty.
 */
struct System
{
    // `windows`, `macos`, `linux`, or Qt's name for another kernel
    QString os;
    // the Windows NT version (10.0.19045), the macOS version (10.15.7), the Linux kernel version (6.1.0)
    QString osVersion;
    // the machine's own architecture: x86_64 for a 32-bit build on 64-bit Windows, arm64 under Rosetta
    QString machineArch;
    // the architecture the running Launcher was built for
    QString launcherArch;
    // Linux only: the versions of glibc and of libstdc++ (its highest GLIBCXX_ symbol version)
    QString glibc;
    QString glibcxx;
    // the version of Qt the Launcher runs on
    QString qt;
};

/**
 * Finds out what the Launcher runs on, once; later calls return the same.
 */
System detect();

/**
 * The architecture a build's `arch` requirement is checked against: on Windows and macOS
 * the machine's, on Linux the Launcher's, since it runs on the system's libraries of the
 * same architecture.
 */
QString requirementArch(const System &system);

/**
 * What a build requires. Empty fields are not given.
 *
 * `os` and `arch` are always needed; `osVersion` on Windows and macOS; `glibc`, `glibcxx`
 * and `qt` on Linux. Requirements without them, or with anything not understood, are never met.
 */
struct Requirements
{
    QString os;
    QString osVersion;
    QString arch;
    QString glibc;
    QString glibcxx;
    // the major version must be the same, the rest is a minimum
    QString qt;
    // requirements this Launcher doesn't know, so it can't check them
    QStringList unknown;
};

/**
 * Checks the requirements against the system. Returns a message for the user for each
 * requirement that isn't met; the list is empty when the system meets them all.
 */
QStringList unmetRequirements(const Requirements &requirements, const System &system);

/**
 * A version is numbers separated by dots, with no leading zeros except a lone 0 (`10.13`, `3.4.21`).
 */
bool isValidVersion(const QString &version);

/**
 * Compares two valid versions numerically, part by part, a missing part counting as 0
 * (`2.19` = `2.19.0` < `2.27`). Returns a negative number, 0 or a positive number when `a`
 * is lower, the same or higher.
 */
int compareVersions(const QString &a, const QString &b);

}
