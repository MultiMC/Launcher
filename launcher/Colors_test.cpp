#include <QTest>
#include "TestUtil.h"

#include "Colors.h"

class ColorsTest : public QObject
{
    Q_OBJECT
private
slots:
    void test_luminance()
    {
        QCOMPARE(Colors::luminance(Qt::black), 0.0);
        QCOMPARE(Colors::luminance(Qt::white), 1.0);
        // WCAG's weights: green counts most
        QVERIFY(Colors::luminance(Qt::green) > Colors::luminance(Qt::red));
        QVERIFY(Colors::luminance(Qt::red) > Colors::luminance(Qt::blue));
    }

    void test_contrastRatio()
    {
        QCOMPARE(Colors::contrastRatio(Qt::black, Qt::white), 21.0);
        QCOMPARE(Colors::contrastRatio(Qt::white, Qt::black), 21.0);
        QCOMPARE(Colors::contrastRatio(QColor("#777777"), QColor("#777777")), 1.0);
    }

    void test_isDark()
    {
        QVERIFY(Colors::isDark(Qt::black));
        QVERIFY(Colors::isDark(QColor(49, 54, 59))); // the Dark theme's window
        QVERIFY(Colors::isDark(QColor(140, 28, 36)));
        QVERIFY(!Colors::isDark(Qt::white));
        QVERIFY(!Colors::isDark(QColor(239, 239, 239))); // Fusion's window
    }

    void test_mix()
    {
        QColor a(0, 0, 0, 0), b(255, 255, 255, 255);
        QCOMPARE(Colors::mix(a, b, 0.0), a);
        QCOMPARE(Colors::mix(a, b, 1.0), b);
        QCOMPARE(Colors::mix(a, b, -1.0), a);
        QCOMPARE(Colors::mix(a, b, 2.0), b);
        QCOMPARE(Colors::mix(a, b, qQNaN()), a);
        QColor middle = Colors::mix(a, b, 0.5);
        QVERIFY(qAbs(middle.red() - 128) <= 1);
        QVERIFY(qAbs(middle.alpha() - 128) <= 1);
    }

    void test_readableUnchangedWhenReadable()
    {
        QCOMPARE(Colors::readable(Qt::black, Qt::white), QColor(Qt::black));
        QCOMPARE(Colors::readable(QColor("purple"), Qt::white), QColor("purple"));
    }

    void test_readableDarkensOnBright()
    {
        QColor orange("orange");
        QColor result = Colors::readable(orange, Qt::white);
        QVERIFY(Colors::contrastRatio(result, Qt::white) >= 4.5);
        QVERIFY(Colors::luminance(result) < Colors::luminance(orange));
        QVERIFY(qAbs(result.hslHue() - orange.hslHue()) <= 2); // 8-bit rounding
        // just enough: not much more than needed
        QVERIFY(Colors::contrastRatio(result, Qt::white) < 4.7);
    }

    void test_readableLightensOnDark()
    {
        QColor background(35, 38, 41); // the Dark theme's base
        QColor purple("purple");
        QColor result = Colors::readable(purple, background);
        QVERIFY(Colors::contrastRatio(result, background) >= 4.5);
        QVERIFY(Colors::luminance(result) > Colors::luminance(purple));
        QVERIFY(qAbs(result.hslHue() - purple.hslHue()) <= 2); // 8-bit rounding
    }

    void test_readableGivesUpAtWhiteAndBlack()
    {
        // nothing reaches 21:1 against grey; the best there is
        QCOMPARE(Colors::readable(QColor("#202020"), QColor("#303030"), 21.0), QColor(Qt::white));
        QCOMPARE(Colors::readable(QColor("#e0e0e0"), QColor("#d0d0d0"), 21.0), QColor(Qt::black));
    }

    void test_readableKeepsAlpha()
    {
        QColor translucent(255, 165, 0, 100);
        QCOMPARE(Colors::readable(translucent, Qt::white).alpha(), 100);
    }

    void test_disabledTakesTheBackgroundsHue()
    {
        // a blue accent on a red window fades to red, not purple
        QColor red(140, 28, 36);
        QColor result = Colors::disabled(QColor(0, 120, 212), red, 0.6);
        QVERIFY(qAbs(result.hslHue() - red.hslHue()) <= 2);
        QCOMPARE(result.hslSaturation(), red.hslSaturation());
    }

    void test_disabledLightnessPartway()
    {
        QColor window(49, 54, 59);
        QCOMPARE(Colors::disabled(Qt::white, window, 0.0).lightness(), 255);
        QVERIFY(qAbs(Colors::disabled(Qt::white, window, 1.0).lightness() - window.lightness()) <= 1);
        int half = Colors::disabled(Qt::white, window, 0.5).lightness();
        QVERIFY(qAbs(half - (255 + window.lightness()) / 2) <= 1);
    }

    void test_accentFromColor()
    {
        QVERIFY(!Colors::Accent::fromColor(QColor()).isValid());
        auto accent = Colors::Accent::fromColor(QColor(0, 120, 212));
        QCOMPARE(accent.color, QColor(0, 120, 212));
        QVERIFY(Colors::luminance(accent.light2) > Colors::luminance(accent.color));
        QVERIFY(Colors::luminance(accent.dark2) < Colors::luminance(accent.color));
        QVERIFY(accent == Colors::Accent::fromColor(QColor(0, 120, 212)));
        QVERIFY(accent != Colors::Accent::fromColor(QColor(200, 0, 0)));
    }
};

QTEST_GUILESS_MAIN(ColorsTest)

#include "Colors_test.moc"
