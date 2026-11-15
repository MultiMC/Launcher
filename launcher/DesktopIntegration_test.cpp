#include <QTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "DesktopIntegration.h"

using DesktopIntegration::State;

// Each test: its own XDG data folders (the user's, and the system's for a package's entry),
// and TMPDIR inside, so the copies made next to it aren't in a temporary folder
class DesktopIntegrationTest : public QObject
{
    Q_OBJECT

    QTemporaryDir *m_base = nullptr;
    QByteArray m_tmpdir;

    QString base() const { return m_base->path(); }
    QString entryPath() const { return base() + "/home/applications/org.multimc.MultiMC.desktop"; }

    // a copy of the Launcher: a folder with an executable start script
    QString copy(const QString &name)
    {
        const QString root = base() + "/" + name;
        QDir().mkpath(root);
        QFile script(root + "/MultiMC");
        script.open(QIODevice::WriteOnly);
        script.write("#!/bin/sh\n");
        script.close();
        script.setPermissions(script.permissions() | QFileDevice::ExeOwner);
        return root;
    }

    QString entry() const
    {
        QFile file(entryPath());
        file.open(QIODevice::ReadOnly);
        return QString::fromUtf8(file.readAll());
    }

private
slots:
    void initTestCase()
    {
        m_tmpdir = qgetenv("TMPDIR");
    }

    void init()
    {
        // the base itself in the real temporary folder: the previous test's TMPDIR is gone
        if (m_tmpdir.isEmpty())
            qunsetenv("TMPDIR");
        else
            qputenv("TMPDIR", m_tmpdir);
        m_base = new QTemporaryDir;
        QVERIFY(m_base->isValid());
        QDir().mkpath(base() + "/tmp");
        qputenv("XDG_DATA_HOME", (base() + "/home").toUtf8());
        qputenv("XDG_DATA_DIRS", (base() + "/system").toUtf8());
        qputenv("TMPDIR", (base() + "/tmp").toUtf8());
    }

    void cleanup()
    {
        delete m_base;
        m_base = nullptr;
    }

    void test_firstCopyWritesItsEntry()
    {
        const QString a = copy("a");
        QCOMPARE(DesktopIntegration::status(a).state, State::Ours);
        DesktopIntegration::update(a, true);
        const QString text = entry();
        QVERIFY(text.contains("Exec=\"" + a + "/MultiMC\"\n"));
        QVERIFY(text.contains("TryExec=" + a + "/MultiMC\n"));
        QVERIFY(text.contains("Icon=org.multimc.MultiMC\n"));
        QVERIFY(text.contains("StartupWMClass=MultiMC\n"));
        QVERIFY(text.contains("NoDisplay=false\n"));
        QVERIFY(text.contains("X-MultiMC-Root=" + a + "\n"));
    }

    void test_hiddenFromTheMenu()
    {
        const QString a = copy("a");
        DesktopIntegration::update(a, false);
        QVERIFY(entry().contains("NoDisplay=true\n"));
        DesktopIntegration::update(a, true);
        QVERIFY(entry().contains("NoDisplay=false\n"));
    }

    void test_anotherCopyLeavesItAlone()
    {
        const QString a = copy("a"), b = copy("b");
        DesktopIntegration::update(a, true);
        DesktopIntegration::update(b, true);
        QVERIFY(entry().contains("X-MultiMC-Root=" + a + "\n"));
        auto status = DesktopIntegration::status(b);
        QCOMPARE(status.state, State::Other);
        QCOMPARE(status.other, a);
    }

    void test_takenOverWhenItsCopyIsGone()
    {
        const QString a = copy("a"), b = copy("b");
        DesktopIntegration::update(a, true);
        QFile::remove(a + "/MultiMC");
        DesktopIntegration::update(b, true);
        QVERIFY(entry().contains("X-MultiMC-Root=" + b + "\n"));
    }

    void test_takeOver()
    {
        const QString a = copy("a"), b = copy("b");
        DesktopIntegration::update(a, true);
        QVERIFY(DesktopIntegration::takeOver(b, false));
        QVERIFY(entry().contains("X-MultiMC-Root=" + b + "\n"));
        QVERIFY(entry().contains("NoDisplay=true\n"));
        QCOMPARE(DesktopIntegration::status(a).state, State::Other);
    }

    void test_anEntryNotOursIsLeftAlone()
    {
        const QString a = copy("a");
        QDir().mkpath(base() + "/home/applications");
        QFile file(entryPath());
        file.open(QIODevice::WriteOnly);
        file.write("[Desktop Entry]\nType=Application\nName=Mine\nExec=/somewhere/else\n");
        file.close();
        DesktopIntegration::update(a, true);
        QVERIFY(entry().contains("Name=Mine\n"));
        auto status = DesktopIntegration::status(a);
        QCOMPARE(status.state, State::Other);
        QVERIFY(status.other.isEmpty());
    }

    void test_aPackageRemovesTheCopysEntry()
    {
        const QString a = copy("a");
        DesktopIntegration::update(a, true);
        QVERIFY(QFile::exists(entryPath()));
        QDir().mkpath(base() + "/system/applications");
        QFile package(base() + "/system/applications/org.multimc.MultiMC.desktop");
        package.open(QIODevice::WriteOnly);
        package.write("[Desktop Entry]\nType=Application\nName=MultiMC 5\nExec=/opt/multimc/run.sh\n");
        package.close();
        QCOMPARE(DesktopIntegration::status(a).state, State::Packaged);
        DesktopIntegration::update(a, true);
        QVERIFY(!QFile::exists(entryPath()));
        QVERIFY(!DesktopIntegration::takeOver(a, true));
    }

    void test_temporaryFolderGetsNothing()
    {
        const QString t = copy("tmp/extracted");
        auto status = DesktopIntegration::status(t);
        QCOMPARE(status.state, State::Unavailable);
        QVERIFY(!status.why.isEmpty());
        DesktopIntegration::update(t, true);
        QVERIFY(!QFile::exists(entryPath()));
    }

    void test_execQuoting()
    {
        const QString odd = copy("with space $HOME \"quoted\"");
        DesktopIntegration::update(odd, true);
        // quoted for Exec ($, " and \ escaped by a backslash), then backslashes doubled as a string
        QString expected = odd;
        expected.replace("$", "\\\\$").replace("\"", "\\\\\"");
        QVERIFY2(entry().contains("Exec=\"" + expected + "/MultiMC\"\n"), qPrintable(entry()));
        // and the root reads back as it was
        QCOMPARE(DesktopIntegration::status(odd).state, State::Ours);
    }
};

QTEST_GUILESS_MAIN(DesktopIntegrationTest)

#include "DesktopIntegration_test.moc"
