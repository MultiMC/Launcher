#include <QTest>
#include <QTemporaryDir>

#include "Colors.h"
#include "FileSystem.h"
#include "Json.h"
#include "ui/themes/CustomTheme.h"
#include "ui/themes/ThemeWriter.h"

class ThemeWriterTest : public QObject
{
    Q_OBJECT

    static QJsonObject readJson(const QString &path)
    {
        return Json::requireObject(Json::requireDocument(path, "test"), "test");
    }

    static ThemeWriter::Theme purple()
    {
        ThemeWriter::Theme theme;
        theme.name = "Purple";
        theme.colors = ThemeWriter::quickColors(QColor("#2b1f3a"), QColor("#f0e8ff"), QColor("#b070ff"));
        theme.logColors["Error"] = QColor("#ff6060");
        return theme;
    }

private
slots:
    // every role set, text readable where it is drawn, both on dark and bright backgrounds
    void test_quickColors_data()
    {
        QTest::addColumn<QColor>("background");
        QTest::addColumn<QColor>("text");
        QTest::addColumn<QColor>("accent");
        QTest::newRow("dark") << QColor("#2b1f3a") << QColor("#f0e8ff") << QColor("#b070ff");
        QTest::newRow("bright") << QColor("#f3f3f3") << QColor("#1a1a1a") << QColor("#0078d4");
        QTest::newRow("bad text") << QColor("#808080") << QColor("#7f7f7f") << QColor("#ffff00");
    }
    void test_quickColors()
    {
        QFETCH(QColor, background);
        QFETCH(QColor, text);
        QFETCH(QColor, accent);
        const auto colors = ThemeWriter::quickColors(background, text, accent);
        for (auto &role : CustomTheme::colorRoles())
        {
            QVERIFY2(colors.value(role.first).isValid(), qPrintable(role.second));
        }
        QCOMPARE(colors[QPalette::Window], background);
        QCOMPARE(colors[QPalette::Highlight], accent);
        QVERIFY(Colors::contrastRatio(colors[QPalette::WindowText], colors[QPalette::Window]) >= 4.5);
        QVERIFY(Colors::contrastRatio(colors[QPalette::Text], colors[QPalette::Base]) >= 4.5);
        QVERIFY(Colors::contrastRatio(colors[QPalette::ButtonText], colors[QPalette::Button]) >= 4.5);
        QVERIFY(Colors::contrastRatio(colors[QPalette::Link], colors[QPalette::Base]) >= 4.5);
        // fields stand apart from the window, away from the text
        QCOMPARE(Colors::isDark(colors[QPalette::Base]), Colors::isDark(background));
        QVERIFY(colors[QPalette::Base] != background);
    }

    void test_newTheme()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString themes = QDir(temp.path()).filePath("themes");
        QString error;
        QVERIFY2(ThemeWriter::save(themes, "Purple", purple(), QString(), error), qPrintable(error));

        CustomTheme theme(themes + "/Purple");
        QVERIFY2(theme.error().isEmpty(), qPrintable(theme.error()));
        QCOMPARE(theme.name(), QString("Purple"));
        QCOMPARE(theme.palette().color(QPalette::Window), QColor("#2b1f3a"));
        QCOMPARE(theme.logColors().value("Error"), QColor("#ff6060"));
        // nothing left next to the themes
        QCOMPARE(QDir(temp.path()).entryList({".theme-new-*"}, QDir::Dirs | QDir::Hidden).size(), 0);
    }

    // a copy takes the stylesheet and images along
    void test_copy()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString themes = QDir(temp.path()).filePath("themes");
        const QString source = themes + "/Original";
        QVERIFY(QDir().mkpath(source + "/resources"));
        FS::write(source + "/theme.json", "{\"name\": \"Original\", \"qssFilePath\": \"style.qss\", \"colors\": {\"Window\": \"#000000\"}}");
        FS::write(source + "/style.qss", "QToolTip { color: red; }");
        FS::write(source + "/resources/image.svg", "<svg/>");

        QString error;
        QVERIFY2(ThemeWriter::save(themes, "Purple", purple(), source, error), qPrintable(error));
        QVERIFY(QFileInfo(themes + "/Purple/resources/image.svg").isFile());
        CustomTheme theme(themes + "/Purple");
        QCOMPARE(theme.name(), QString("Purple"));
        QCOMPARE(theme.styleSheet(), QString("QToolTip { color: red; }"));
        QCOMPARE(theme.palette().color(QPalette::Window), QColor("#2b1f3a"));
        // the original as it was
        QCOMPARE(CustomTheme(source).palette().color(QPalette::Window), QColor("#000000"));
    }

    // in place: what we don't write stays, what we ignore goes
    void test_editInPlace()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString themes = QDir(temp.path()).filePath("themes");
        QVERIFY(QDir().mkpath(themes + "/Purple"));
        FS::write(themes + "/Purple/theme.json",
                  "{\"name\": \"Old\", \"widgets\": \"Fusion\", \"qssFilePath\": \"x.css\", \"future\": 1,"
                  " \"colors\": {\"Window\": \"#000000\", \"fadeAmount\": 0.5}, \"logColors\": {\"Debug\": \"#00ff00\"}}");

        ThemeWriter::Theme edited = purple();
        QString error;
        QVERIFY2(ThemeWriter::save(themes, "Purple", edited, QString(), error), qPrintable(error));
        const QJsonObject root = readJson(themes + "/Purple/theme.json");
        QCOMPARE(root.value("name").toString(), QString("Purple"));
        QCOMPARE(root.value("qssFilePath").toString(), QString("x.css"));
        QCOMPARE(root.value("future").toInt(), 1);
        QVERIFY(!root.contains("widgets"));
        QVERIFY(!root.value("colors").toObject().contains("fadeAmount"));
        QCOMPARE(root.value("colors").toObject().value("Window").toString(), QString("#2b1f3a"));
        // the log colours are the editor's: Debug was reset
        QCOMPARE(root.value("logColors").toObject().keys(), QStringList{"Error"});
    }

    // the accent and the title bar: themes from before the keys keep their look (their own
    // accent, the window's colours on title bars); the keys are read, kept when the editor
    // saves without them, and written when it gives them
    void test_accentAndTitleBar()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString themes = QDir(temp.path()).filePath("themes");
        QVERIFY(QDir().mkpath(themes + "/Old"));
        FS::write(themes + "/Old/theme.json", "{\"name\": \"Old\", \"colors\": {\"Window\": \"#282828\"}}");
        CustomTheme old(themes + "/Old");
        QCOMPARE(old.accentMode(), AccentMode::Custom);
        QCOMPARE(old.titleBarMode(), TitleBarMode::Window);
        QVERIFY(!old.titleBarColor().isValid());

        QVERIFY(QDir().mkpath(themes + "/New"));
        FS::write(themes + "/New/theme.json",
                  "{\"name\": \"New\", \"accent\": \"system\", \"titleBar\": \"custom\","
                  " \"colors\": {\"TitleBar\": \"#102030\", \"TitleBarText\": \"#f0f0f0\"}}");
        CustomTheme fresh(themes + "/New");
        QCOMPARE(fresh.accentMode(), AccentMode::System);
        QCOMPARE(fresh.titleBarMode(), TitleBarMode::Custom);
        QCOMPARE(fresh.titleBarColor(), QColor("#102030"));
        QCOMPARE(fresh.titleBarTextColor(), QColor("#f0f0f0"));

        // saved without the modes: they stay, with the title bar's colours
        QString error;
        QVERIFY2(ThemeWriter::save(themes, "New", purple(), QString(), error), qPrintable(error));
        fresh.reload();
        QCOMPARE(fresh.accentMode(), AccentMode::System);
        QCOMPARE(fresh.titleBarMode(), TitleBarMode::Custom);
        QCOMPARE(fresh.titleBarColor(), QColor("#102030"));

        // saved with them: as given; the title bar's colours go with a mode that has none
        ThemeWriter::Theme edited = purple();
        edited.accent = CustomTheme::accentModeName(AccentMode::Custom);
        edited.titleBar = CustomTheme::titleBarModeName(TitleBarMode::Accent);
        QVERIFY2(ThemeWriter::save(themes, "New", edited, QString(), error), qPrintable(error));
        fresh.reload();
        QCOMPARE(fresh.accentMode(), AccentMode::Custom);
        QCOMPARE(fresh.titleBarMode(), TitleBarMode::Accent);
        QVERIFY(!fresh.titleBarColor().isValid());

        // a copy of a built-in theme: what that theme does
        ThemeWriter::Theme copy = purple();
        copy.accent = CustomTheme::accentModeName(AccentMode::System);
        copy.titleBar = CustomTheme::titleBarModeName(TitleBarMode::System);
        QVERIFY2(ThemeWriter::save(themes, "Copy", copy, QString(), error), qPrintable(error));
        QCOMPARE(CustomTheme(themes + "/Copy").accentMode(), AccentMode::System);
        QCOMPARE(CustomTheme(themes + "/Copy").titleBarMode(), TitleBarMode::System);

        // something else: the old look
        QVERIFY(QDir().mkpath(themes + "/Odd"));
        FS::write(themes + "/Odd/theme.json", "{\"name\": \"Odd\", \"accent\": \"maybe\", \"titleBar\": \"purple\"}");
        QCOMPARE(CustomTheme(themes + "/Odd").accentMode(), AccentMode::Custom);
        QCOMPARE(CustomTheme(themes + "/Odd").titleBarMode(), TitleBarMode::Window);
    }

    void test_freeId()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString themes = QDir(temp.path()).filePath("themes");
        QVERIFY(QDir().mkpath(themes + "/Purple"));
        QCOMPARE(ThemeWriter::freeId(themes, "Purple", {}), QString("Purple 2"));
        QCOMPARE(ThemeWriter::freeId(themes, "Green", {}), QString("Green"));
        QCOMPARE(ThemeWriter::freeId(themes, "Dark", {"dark"}), QString("Dark 2"));
        QCOMPARE(ThemeWriter::freeId(themes, "a/b", {}), QString("a-b"));
        QCOMPARE(ThemeWriter::freeId(themes, "..", {}), QString("theme"));
    }
};

QTEST_GUILESS_MAIN(ThemeWriterTest)

#include "ThemeWriter_test.moc"
