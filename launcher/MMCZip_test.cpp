#include <QTest>
#include <QTemporaryDir>

#include <quazip.h>
#include <quazipfile.h>

#include "MMCZip.h"

class MMCZipTest : public QObject
{
    Q_OBJECT

    // a zip with these entries, each holding its own name
    static void makeZip(const QString &path, const QStringList &entries)
    {
        QuaZip zip(path);
        QVERIFY(zip.open(QuaZip::mdCreate));
        for (auto &entry : entries)
        {
            QuaZipFile file(&zip);
            QVERIFY(file.open(QIODevice::WriteOnly, QuaZipNewInfo(entry)));
            if (!entry.endsWith('/'))
            {
                file.write(entry.toUtf8());
            }
            file.close();
        }
        zip.close();
        QCOMPARE(zip.getZipError(), 0);
    }

private
slots:
    void test_extractDir()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString zipPath = QDir(temp.path()).filePath("good.zip");
        makeZip(zipPath, {"folder/", "folder/file.txt", "top.txt"});
        const QString target = QDir(temp.path()).filePath("target");

        auto extracted = MMCZip::extractDir(zipPath, target);
        QVERIFY(extracted.has_value());
        QVERIFY(QFileInfo(target + "/folder").isDir());
        QVERIFY(QFileInfo(target + "/folder/file.txt").isFile());
        QVERIFY(QFileInfo(target + "/top.txt").isFile());
    }

    void test_extractDirRefusesEntriesOutside_data()
    {
        QTest::addColumn<QString>("entry");
        QTest::newRow("up") << "../escaped.txt";
        QTest::newRow("up from a folder") << "folder/../../escaped.txt";
    }
    // "zip slip": an archive must not write outside of where it is extracted
    void test_extractDirRefusesEntriesOutside()
    {
        QFETCH(QString, entry);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString zipPath = QDir(temp.path()).filePath("evil.zip");
        makeZip(zipPath, {"fine.txt", entry});
        const QString target = QDir(temp.path()).filePath("deep/target");

        auto extracted = MMCZip::extractDir(zipPath, target);
        QVERIFY(!extracted.has_value());
        QVERIFY(!QFileInfo(QDir(temp.path()).filePath("deep/escaped.txt")).exists());
        QVERIFY(!QFileInfo(QDir(temp.path()).filePath("escaped.txt")).exists());
        // what was extracted before it is removed again
        QVERIFY(!QFileInfo(target + "/fine.txt").exists());
    }
};

QTEST_GUILESS_MAIN(MMCZipTest)

#include "MMCZip_test.moc"
