#include <QTest>

#include "TestUtil.h"
#include "updater/v2/SystemRequirements.h"

using SystemRequirements::Requirements;
using SystemRequirements::System;

namespace {

System linuxSystem()
{
    System system;
    system.os = "linux";
    system.osVersion = "6.1.0";
    system.machineArch = "x86_64";
    system.launcherArch = "x86_64";
    system.glibc = "2.27";
    system.glibcxx = "3.4.25";
    system.qt = "5.9.5";
    return system;
}

Requirements linuxRequirements()
{
    Requirements requirements;
    requirements.os = "linux";
    requirements.arch = "x86_64";
    requirements.glibc = "2.19";
    requirements.glibcxx = "3.4.21";
    requirements.qt = "5.6";
    return requirements;
}

System windowsSystem()
{
    System system;
    system.os = "windows";
    system.osVersion = "10.0.19045";
    system.machineArch = "x86_64";
    system.launcherArch = "x86_64";
    system.qt = "5.6.3";
    return system;
}

Requirements windowsRequirements()
{
    Requirements requirements;
    requirements.os = "windows";
    requirements.osVersion = "5.1";
    requirements.arch = "x86";
    return requirements;
}

System macSystem()
{
    System system;
    system.os = "macos";
    system.osVersion = "10.15.7";
    system.machineArch = "x86_64";
    system.launcherArch = "x86_64";
    system.qt = "5.15.2";
    return system;
}

Requirements macRequirements()
{
    Requirements requirements;
    requirements.os = "macos";
    requirements.osVersion = "10.13";
    requirements.arch = "x86_64";
    return requirements;
}

}

class SystemRequirementsTest : public QObject
{
    Q_OBJECT
private
slots:
    void test_isValidVersion_data()
    {
        QTest::addColumn<QString>("version");
        QTest::addColumn<bool>("valid");
        QTest::newRow("one part") << QString("5") << true;
        QTest::newRow("two parts") << QString("10.13") << true;
        QTest::newRow("three parts") << QString("3.4.21") << true;
        QTest::newRow("zero") << QString("0") << true;
        QTest::newRow("zero part") << QString("10.0.19045") << true;
        QTest::newRow("empty") << QString("") << false;
        QTest::newRow("leading zero") << QString("10.01") << false;
        QTest::newRow("empty part") << QString("2..19") << false;
        QTest::newRow("trailing dot") << QString("2.19.") << false;
        QTest::newRow("leading dot") << QString(".19") << false;
        QTest::newRow("suffix") << QString("5.15.2-1") << false;
        QTest::newRow("letters") << QString("5.x") << false;
        QTest::newRow("sign") << QString("-5") << false;
        QTest::newRow("space") << QString(" 5") << false;
        QTest::newRow("too long a part") << QString("1234567890") << false;
        QTest::newRow("long enough a part") << QString("123456789") << true;
    }
    void test_isValidVersion()
    {
        QFETCH(QString, version);
        QFETCH(bool, valid);
        QCOMPARE(SystemRequirements::isValidVersion(version), valid);
    }

    void test_compareVersions_data()
    {
        QTest::addColumn<QString>("a");
        QTest::addColumn<QString>("b");
        QTest::addColumn<int>("result");
        QTest::newRow("same") << QString("2.19") << QString("2.19") << 0;
        QTest::newRow("missing part is 0") << QString("2.19") << QString("2.19.0") << 0;
        QTest::newRow("missing parts are 0") << QString("5") << QString("5.0.0") << 0;
        QTest::newRow("lower") << QString("2.19") << QString("2.27") << -1;
        QTest::newRow("higher") << QString("2.27") << QString("2.19") << 1;
        QTest::newRow("numeric, not text") << QString("3.4.9") << QString("3.4.21") << -1;
        QTest::newRow("more parts, higher") << QString("10.13.1") << QString("10.13") << 1;
        QTest::newRow("major decides") << QString("11") << QString("10.15.7") << 1;
    }
    void test_compareVersions()
    {
        QFETCH(QString, a);
        QFETCH(QString, b);
        QFETCH(int, result);
        int compared = SystemRequirements::compareVersions(a, b);
        QCOMPARE(compared < 0 ? -1 : compared > 0 ? 1 : 0, result);
    }

    void test_met()
    {
        QCOMPARE(SystemRequirements::unmetRequirements(linuxRequirements(), linuxSystem()), QStringList());
        QCOMPARE(SystemRequirements::unmetRequirements(windowsRequirements(), windowsSystem()), QStringList());
        QCOMPARE(SystemRequirements::unmetRequirements(macRequirements(), macSystem()), QStringList());
    }

    void test_exactMinimumsAreMet()
    {
        auto system = linuxSystem();
        auto requirements = linuxRequirements();
        requirements.glibc = system.glibc;
        requirements.glibcxx = system.glibcxx;
        requirements.qt = system.qt;
        QCOMPARE(SystemRequirements::unmetRequirements(requirements, system), QStringList());
        system.glibc = "2.19.0";
        requirements.glibc = "2.19";
        QCOMPARE(SystemRequirements::unmetRequirements(requirements, system), QStringList());
    }

    void test_otherOs()
    {
        auto unmet = SystemRequirements::unmetRequirements(windowsRequirements(), linuxSystem());
        QCOMPARE(unmet.size(), 1);
        QVERIFY(unmet[0].contains("Windows"));
        QCOMPARE(SystemRequirements::unmetRequirements(macRequirements(), windowsSystem()).size(), 1);
        QCOMPARE(SystemRequirements::unmetRequirements(linuxRequirements(), macSystem()).size(), 1);
    }

    void test_arch_data()
    {
        QTest::addColumn<QString>("os");
        QTest::addColumn<QString>("machine");
        QTest::addColumn<QString>("build");
        QTest::addColumn<bool>("met");
        QTest::newRow("windows x86 on x86") << QString("windows") << QString("x86") << QString("x86") << true;
        QTest::newRow("windows x86 on x86_64") << QString("windows") << QString("x86_64") << QString("x86") << true;
        QTest::newRow("windows x86_64 on x86") << QString("windows") << QString("x86") << QString("x86_64") << false;
        QTest::newRow("windows x86_64 on arm64") << QString("windows") << QString("arm64") << QString("x86_64") << false;
        QTest::newRow("macos x86_64 on x86_64") << QString("macos") << QString("x86_64") << QString("x86_64") << true;
        QTest::newRow("macos x86_64 on arm64 (Rosetta)") << QString("macos") << QString("arm64") << QString("x86_64") << true;
        QTest::newRow("macos arm64 on x86_64") << QString("macos") << QString("x86_64") << QString("arm64") << false;
        QTest::newRow("linux x86_64 on x86_64") << QString("linux") << QString("x86_64") << QString("x86_64") << true;
        QTest::newRow("linux x86 on x86") << QString("linux") << QString("x86") << QString("x86") << true;
        QTest::newRow("linux x86 on x86_64") << QString("linux") << QString("x86_64") << QString("x86") << false;
        QTest::newRow("linux x86_64 on x86") << QString("linux") << QString("x86") << QString("x86_64") << false;
        QTest::newRow("unknown machine") << QString("linux") << QString("") << QString("x86_64") << false;
    }
    void test_arch()
    {
        QFETCH(QString, os);
        QFETCH(QString, machine);
        QFETCH(QString, build);
        QFETCH(bool, met);
        System system;
        Requirements requirements;
        if(os == "windows")
        {
            system = windowsSystem();
            requirements = windowsRequirements();
        }
        else if(os == "macos")
        {
            system = macSystem();
            requirements = macRequirements();
        }
        else
        {
            system = linuxSystem();
            requirements = linuxRequirements();
        }
        // what the requirement is checked against; the other one must not matter
        if(os == "linux")
        {
            system.launcherArch = machine;
            system.machineArch = "arm64";
        }
        else
        {
            system.machineArch = machine;
            system.launcherArch = "arm64";
        }
        requirements.arch = build;
        QCOMPARE(SystemRequirements::unmetRequirements(requirements, system).isEmpty(), met);
    }

    void test_osVersion()
    {
        auto requirements = windowsRequirements();
        requirements.osVersion = "6.1";
        auto system = windowsSystem();
        system.osVersion = "5.1.2600";
        auto unmet = SystemRequirements::unmetRequirements(requirements, system);
        QCOMPARE(unmet.size(), 1);
        QVERIFY(unmet[0].contains("Windows 7"));
        QVERIFY(unmet[0].contains("Windows XP"));

        auto macRequired = macRequirements();
        auto mac = macSystem();
        mac.osVersion = "10.12.6";
        unmet = SystemRequirements::unmetRequirements(macRequired, mac);
        QCOMPARE(unmet.size(), 1);
        QVERIFY(unmet[0].contains("macOS 10.13"));
        mac.osVersion = "11.7";
        QCOMPARE(SystemRequirements::unmetRequirements(macRequired, mac), QStringList());
    }

    void test_linuxLibraries()
    {
        auto system = linuxSystem();
        auto requirements = linuxRequirements();
        requirements.glibc = "2.28";
        requirements.glibcxx = "3.4.26";
        requirements.qt = "5.12";
        auto unmet = SystemRequirements::unmetRequirements(requirements, system);
        QCOMPARE(unmet.size(), 3);
        QVERIFY(unmet[0].contains("glibc 2.28"));
        QVERIFY(unmet[1].contains("GLIBCXX_3.4.26"));
        QVERIFY(unmet[2].contains("Qt 5.12"));
    }

    void test_qtMajorMustMatch()
    {
        auto system = linuxSystem();
        auto requirements = linuxRequirements();
        system.qt = "6.5.0";
        QCOMPARE(SystemRequirements::unmetRequirements(requirements, system).size(), 1);
        system.qt = "4.8.7";
        QCOMPARE(SystemRequirements::unmetRequirements(requirements, system).size(), 1);
        system.qt = "5.6.0";
        QCOMPARE(SystemRequirements::unmetRequirements(requirements, system), QStringList());
        system.qt = "5.15.2";
        QCOMPARE(SystemRequirements::unmetRequirements(requirements, system), QStringList());
    }

    void test_undetectedValuesFail()
    {
        auto system = linuxSystem();
        system.glibc.clear();
        system.glibcxx.clear();
        system.qt.clear();
        QCOMPARE(SystemRequirements::unmetRequirements(linuxRequirements(), system).size(), 3);

        auto windows = windowsSystem();
        windows.osVersion.clear();
        QCOMPARE(SystemRequirements::unmetRequirements(windowsRequirements(), windows).size(), 1);
    }

    void test_incompleteRequirementsFail()
    {
        Requirements empty;
        QCOMPARE(SystemRequirements::unmetRequirements(empty, linuxSystem()).size(), 1);

        auto unknownOs = linuxRequirements();
        unknownOs.os = "freebsd";
        QCOMPARE(SystemRequirements::unmetRequirements(unknownOs, linuxSystem()).size(), 1);

        auto unknownRequirement = linuxRequirements();
        unknownRequirement.unknown << "gpu";
        QCOMPARE(SystemRequirements::unmetRequirements(unknownRequirement, linuxSystem()).size(), 1);

        auto noArch = linuxRequirements();
        noArch.arch.clear();
        QCOMPARE(SystemRequirements::unmetRequirements(noArch, linuxSystem()).size(), 1);

        auto noLibraries = linuxRequirements();
        noLibraries.glibc.clear();
        noLibraries.glibcxx.clear();
        noLibraries.qt.clear();
        QCOMPARE(SystemRequirements::unmetRequirements(noLibraries, linuxSystem()).size(), 3);

        auto badVersion = linuxRequirements();
        badVersion.glibc = "2.19a";
        QCOMPARE(SystemRequirements::unmetRequirements(badVersion, linuxSystem()).size(), 1);

        auto noOsVersion = windowsRequirements();
        noOsVersion.osVersion.clear();
        QCOMPARE(SystemRequirements::unmetRequirements(noOsVersion, windowsSystem()).size(), 1);
    }

    void test_detect()
    {
        auto system = SystemRequirements::detect();
        qDebug() << "os" << system.os << system.osVersion << "machine" << system.machineArch << "launcher" << system.launcherArch
                 << "glibc" << system.glibc << "glibcxx" << system.glibcxx << "qt" << system.qt;
        QCOMPARE(system.qt, QString::fromLatin1(qVersion()));
        QVERIFY(SystemRequirements::isValidVersion(system.osVersion));
        QStringList known = {"x86", "x86_64", "arm64"};
        QVERIFY(known.contains(system.machineArch));
        QVERIFY(known.contains(system.launcherArch));
#if defined(Q_PROCESSOR_X86_64)
        QCOMPARE(system.launcherArch, QString("x86_64"));
#elif defined(Q_PROCESSOR_X86_32)
        QCOMPARE(system.launcherArch, QString("x86"));
#endif
#if defined(Q_OS_WIN)
        QCOMPARE(system.os, QString("windows"));
        QVERIFY(system.glibc.isEmpty());
#elif defined(Q_OS_MAC)
        QCOMPARE(system.os, QString("macos"));
        QVERIFY(system.glibc.isEmpty());
#elif defined(Q_OS_LINUX)
        QCOMPARE(system.os, QString("linux"));
        QVERIFY(SystemRequirements::isValidVersion(system.glibc));
        QVERIFY(SystemRequirements::isValidVersion(system.glibcxx));
        Requirements requirements;
        requirements.os = "linux";
        requirements.arch = system.launcherArch;
        requirements.glibc = system.glibc;
        requirements.glibcxx = system.glibcxx;
        requirements.qt = system.qt;
        QCOMPARE(SystemRequirements::unmetRequirements(requirements, system), QStringList());
#endif
    }
};

QTEST_GUILESS_MAIN(SystemRequirementsTest)

#include "SystemRequirements_test.moc"
