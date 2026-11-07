#include <QTest>
#include <QTemporaryDir>
#include <QStandardPaths>
#include "TestUtil.h"

#include "FileSystem.h"

class FileSystemTest : public QObject
{
    Q_OBJECT

    const QString bothSlash = "/foo/";
    const QString trailingSlash = "foo/";
    const QString leadingSlash = "/foo";

private
slots:
    void test_pathCombine()
    {
        QCOMPARE(QString("/foo/foo"), FS::PathCombine(bothSlash, bothSlash));
        QCOMPARE(QString("foo/foo"), FS::PathCombine(trailingSlash, trailingSlash));
        QCOMPARE(QString("/foo/foo"), FS::PathCombine(leadingSlash, leadingSlash));

        QCOMPARE(QString("/foo/foo/foo"), FS::PathCombine(bothSlash, bothSlash, bothSlash));
        QCOMPARE(QString("foo/foo/foo"), FS::PathCombine(trailingSlash, trailingSlash, trailingSlash));
        QCOMPARE(QString("/foo/foo/foo"), FS::PathCombine(leadingSlash, leadingSlash, leadingSlash));
    }

    void test_PathInside_data()
    {
        QTest::addColumn<QString>("relative");
        QTest::addColumn<QString>("inside");

        QTest::newRow("file") << "a.txt" << "a.txt";
        QTest::newRow("nested") << "a/b/c.txt" << "a/b/c.txt";
        QTest::newRow("folder entry") << "a/b/" << "a/b";
        QTest::newRow("up and back in") << "a/../b.txt" << "b.txt";
        QTest::newRow("the folder itself") << "" << "";
        QTest::newRow("dot") << "./a.txt" << "a.txt";
        QTest::newRow("up") << "../a.txt" << QString();
        QTest::newRow("up from nested") << "a/../../a.txt" << QString();
        QTest::newRow("up a lot") << "a/b/../../../../../a.txt" << QString();
        QTest::newRow("absolute") << "/etc/passwd" << QString();
        // a folder next to it whose name starts the same: /tmp/abc vs /tmp/abc-sibling
        QTest::newRow("sibling with the same start") << "../%ROOT%-sibling/a.txt" << QString();
    }
    void test_PathInside()
    {
        QFETCH(QString, relative);
        QFETCH(QString, inside);

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString root = QDir::cleanPath(QDir(tempDir.path()).absolutePath());
        relative.replace("%ROOT%", QFileInfo(root).fileName());
        auto result = FS::PathInside(tempDir.path(), relative);
        if (inside.isNull())
        {
            QVERIFY(!result);
            return;
        }
        QVERIFY(bool(result));
        QCOMPARE(*result, inside.isEmpty() ? root : root + "/" + inside);
    }

    void test_PathCombine1_data()
    {
        QTest::addColumn<QString>("result");
        QTest::addColumn<QString>("path1");
        QTest::addColumn<QString>("path2");

        QTest::newRow("qt 1") << "/abc/def/ghi/jkl" << "/abc/def" << "ghi/jkl";
        QTest::newRow("qt 2") << "/abc/def/ghi/jkl" << "/abc/def/" << "ghi/jkl";
#if defined(Q_OS_WIN)
        QTest::newRow("win native, from C:") << "C:/abc" << "C:" << "abc";
        QTest::newRow("win native 1") << "C:/abc/def/ghi/jkl" << "C:\\abc\\def" << "ghi\\jkl";
        QTest::newRow("win native 2") << "C:/abc/def/ghi/jkl" << "C:\\abc\\def\\" << "ghi\\jkl";
#endif
    }

    void test_PathCombine1()
    {
        QFETCH(QString, result);
        QFETCH(QString, path1);
        QFETCH(QString, path2);

        QCOMPARE(FS::PathCombine(path1, path2), result);
    }

    void test_PathCombine2_data()
    {
        QTest::addColumn<QString>("result");
        QTest::addColumn<QString>("path1");
        QTest::addColumn<QString>("path2");
        QTest::addColumn<QString>("path3");

        QTest::newRow("qt 1") << "/abc/def/ghi/jkl" << "/abc" << "def" << "ghi/jkl";
        QTest::newRow("qt 2") << "/abc/def/ghi/jkl" << "/abc/" << "def" << "ghi/jkl";
        QTest::newRow("qt 3") << "/abc/def/ghi/jkl" << "/abc" << "def/" << "ghi/jkl";
        QTest::newRow("qt 4") << "/abc/def/ghi/jkl" << "/abc/" << "def/" << "ghi/jkl";
#if defined(Q_OS_WIN)
        QTest::newRow("win 1") << "C:/abc/def/ghi/jkl" << "C:\\abc" << "def" << "ghi\\jkl";
        QTest::newRow("win 2") << "C:/abc/def/ghi/jkl" << "C:\\abc\\" << "def" << "ghi\\jkl";
        QTest::newRow("win 3") << "C:/abc/def/ghi/jkl" << "C:\\abc" << "def\\" << "ghi\\jkl";
        QTest::newRow("win 4") << "C:/abc/def/ghi/jkl" << "C:\\abc\\" << "def" << "ghi\\jkl";
#endif
    }

    void test_PathCombine2()
    {
        QFETCH(QString, result);
        QFETCH(QString, path1);
        QFETCH(QString, path2);
        QFETCH(QString, path3);

        QCOMPARE(FS::PathCombine(path1, path2, path3), result);
    }

    void test_copy()
    {
        QString folder = QFINDTESTDATA("data/test_folder");
        auto f = [&folder]()
        {
            QTemporaryDir tempDir;
            tempDir.setAutoRemove(true);
            qDebug() << "From:" << folder << "To:" << tempDir.path();

            QDir target_dir(FS::PathCombine(tempDir.path(), "test_folder"));
            qDebug() << tempDir.path();
            qDebug() << target_dir.path();
            FS::copy c(folder, target_dir.path());
            c();

            for(auto entry: target_dir.entryList())
            {
                qDebug() << entry;
            }
            QVERIFY(target_dir.entryList().contains("pack.mcmeta"));
            QVERIFY(target_dir.entryList().contains("assets"));
        };

        // first try variant without trailing /
        QVERIFY(!folder.endsWith('/'));
        f();

        // then variant with trailing /
        folder.append('/');
        QVERIFY(folder.endsWith('/'));
        f();
    }

    void test_getDesktop()
    {
        QCOMPARE(FS::getDesktopDir(), QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    }

// this is only valid on linux
// FIXME: implement on windows, OSX, then test.
#if defined(Q_OS_LINUX)
    void test_createShortcut_data()
    {
        QTest::addColumn<QString>("location");
        QTest::addColumn<QString>("dest");
        QTest::addColumn<QStringList>("args");
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("iconLocation");
        QTest::addColumn<QByteArray>("result");

        QTest::newRow("unix") << QDir::currentPath()
                              << "asdfDest"
                              << (QStringList() << "arg1" << "arg2")
                              << "asdf"
                              << QString()
                         #if defined(Q_OS_LINUX)
                              << GET_TEST_FILE("data/FileSystem-test_createShortcut-unix")
                         #elif defined(Q_OS_WIN)
                              << QByteArray()
                         #endif
                                 ;
    }

    void test_createShortcut()
    {
        QFETCH(QString, location);
        QFETCH(QString, dest);
        QFETCH(QStringList, args);
        QFETCH(QString, name);
        QFETCH(QString, iconLocation);
        QFETCH(QByteArray, result);

        QVERIFY(FS::createShortCut(location, dest, args, name, iconLocation));
        QCOMPARE(QString::fromLocal8Bit(TestsInternal::readFile(location + QDir::separator() + name + ".desktop")), QString::fromLocal8Bit(result));

        //QDir().remove(location);
    }
#endif
};

QTEST_GUILESS_MAIN(FileSystemTest)

#include "FileSystem_test.moc"
