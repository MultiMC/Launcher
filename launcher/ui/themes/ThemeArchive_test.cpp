#include <QTest>
#include <QTemporaryDir>

#include <quazip.h>
#include <quazipfile.h>

#include "ui/themes/ThemeArchive.h"

class ThemeArchiveTest : public QObject
{
    Q_OBJECT

    // a zip with these entries and contents (folders end with a slash and have none)
    static void makeZip(const QString &path, const QMap<QString, QByteArray> &entries)
    {
        QuaZip zip(path);
        QVERIFY(zip.open(QuaZip::mdCreate));
        for (auto it = entries.begin(); it != entries.end(); ++it)
        {
            QuaZipFile file(&zip);
            QVERIFY(file.open(QIODevice::WriteOnly, QuaZipNewInfo(it.key())));
            file.write(it.value());
            file.close();
        }
        zip.close();
        QCOMPARE(zip.getZipError(), 0);
    }

    const QByteArray THEME_JSON = R"({"name": "Purple", "colors": {"Window": "#2b1f3a"}})";
    const QByteArray INDEX_THEME = "[Icon Theme]\nName=Pixels\n";

private
slots:
    void test_inspect_data()
    {
        QTest::addColumn<QStringList>("entries");
        QTest::addColumn<bool>("valid");
        QTest::addColumn<int>("kind");
        QTest::addColumn<QString>("subdir");
        QTest::addColumn<QString>("id");
        QTest::newRow("colours at the top") << QStringList{"theme.json", "themeStyle.css"} << true << int(ThemeArchive::Kind::Colors) << "" << "archive";
        QTest::newRow("colours in a folder") << QStringList{"Purple/", "Purple/theme.json"} << true << int(ThemeArchive::Kind::Colors) << "Purple/" << "Purple";
        QTest::newRow("icons in a folder") << QStringList{"pixels/index.theme", "pixels/scalable/news.svg"} << true << int(ThemeArchive::Kind::Icons) << "pixels/" << "pixels";
        QTest::newRow("nothing") << QStringList{"readme.txt"} << false << 0 << "" << "";
        QTest::newRow("too deep") << QStringList{"a/b/theme.json"} << false << 0 << "" << "";
        QTest::newRow("two themes") << QStringList{"a/theme.json", "b/theme.json"} << false << 0 << "" << "";
        QTest::newRow("hidden folder") << QStringList{".x/theme.json"} << false << 0 << "" << "";
    }
    void test_inspect()
    {
        QFETCH(QStringList, entries);
        QFETCH(bool, valid);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString zipPath = QDir(temp.path()).filePath("archive.zip");
        QMap<QString, QByteArray> files;
        for (auto &entry : entries)
            files[entry] = entry.endsWith('/') ? QByteArray() : QByteArray("x");
        makeZip(zipPath, files);

        QString error;
        auto contents = ThemeArchive::inspect(zipPath, error);
        QCOMPARE(contents.has_value(), valid);
        if (!valid)
        {
            QVERIFY(!error.isEmpty());
            return;
        }
        QFETCH(int, kind);
        QFETCH(QString, subdir);
        QFETCH(QString, id);
        QCOMPARE(int(contents->kind), kind);
        QCOMPARE(contents->subdir, subdir);
        QCOMPARE(contents->id, id);
    }

    void test_installReplacesAndExportRoundTrips()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString themes = QDir(temp.path()).filePath("iconthemes");
        const QString zipPath = QDir(temp.path()).filePath("pixels.zip");
        makeZip(zipPath, {{"pixels/index.theme", INDEX_THEME}, {"pixels/scalable/news.svg", "<svg/>"}});

        // something old in the way: replaced, not merged
        QVERIFY(QDir().mkpath(themes + "/pixels"));
        QFile old(themes + "/pixels/old.txt");
        QVERIFY(old.open(QIODevice::WriteOnly));
        old.close();

        QString error;
        auto contents = ThemeArchive::inspect(zipPath, error);
        QVERIFY(contents.has_value());
        QVERIFY2(ThemeArchive::install(zipPath, *contents, themes, error), qPrintable(error));
        QVERIFY(QFileInfo(themes + "/pixels/index.theme").isFile());
        QVERIFY(QFileInfo(themes + "/pixels/scalable/news.svg").isFile());
        QVERIFY(!QFileInfo(themes + "/pixels/old.txt").exists());
        // the temporary folder is gone
        QCOMPARE(QDir(temp.path()).entryList({".theme-import-*"}, QDir::Dirs | QDir::Hidden).size(), 0);

        // out again, under another name: the folder inside keeps the id
        const QString exported = QDir(temp.path()).filePath("shared.zip");
        QVERIFY2(ThemeArchive::exportTo(themes + "/pixels", exported, error), qPrintable(error));
        auto again = ThemeArchive::inspect(exported, error);
        QVERIFY(again.has_value());
        QCOMPARE(int(again->kind), int(ThemeArchive::Kind::Icons));
        QCOMPARE(again->id, QString("pixels"));
    }

    void test_installColorTheme()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString themes = QDir(temp.path()).filePath("themes");
        const QString zipPath = QDir(temp.path()).filePath("Purple.zip");
        makeZip(zipPath, {{"theme.json", THEME_JSON}});

        QString error;
        auto contents = ThemeArchive::inspect(zipPath, error);
        QVERIFY(contents.has_value());
        QVERIFY2(ThemeArchive::install(zipPath, *contents, themes, error), qPrintable(error));
        QVERIFY(QFileInfo(themes + "/Purple/theme.json").isFile());
    }

    // a theme.json that doesn't read never lands in themes/
    void test_installRefusesBrokenColorTheme()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString themes = QDir(temp.path()).filePath("themes");
        const QString zipPath = QDir(temp.path()).filePath("broken.zip");
        makeZip(zipPath, {{"broken/theme.json", "{ not json"}});

        QString error;
        auto contents = ThemeArchive::inspect(zipPath, error);
        QVERIFY(contents.has_value());
        QVERIFY(!ThemeArchive::install(zipPath, *contents, themes, error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!QFileInfo(themes + "/broken").exists());
    }
};

QTEST_GUILESS_MAIN(ThemeArchiveTest)

#include "ThemeArchive_test.moc"
