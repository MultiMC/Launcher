#include <QTest>

#include "updater/v2/CheckHistory.h"

using namespace UpdaterV2;

namespace {
const qint64 Day = 24 * 60 * 60;
const qint64 Start = 1800000000;
}

class UpdaterV2CheckHistoryTest : public QObject
{
    Q_OBJECT

private slots:
    void test_success()
    {
        CheckHistory history;
        QVERIFY(!history.record(true, Start));
        QCOMPARE(history.lastSuccess, Start);
        QCOMPARE(history.failingSince, qint64(0));
        QCOMPARE(history.failedChecks, 0);
    }

    void test_failingForLong()
    {
        CheckHistory history;
        history.record(true, Start);
        // every day for two weeks: nothing yet
        for(int day = 1; day <= 14; day++)
        {
            QVERIFY(!history.record(false, Start + day * Day));
        }
        QCOMPARE(history.failingSince, Start + Day);
        QCOMPARE(history.failedChecks, 14);
        // two weeks after the first failure
        QVERIFY(history.record(false, Start + 15 * Day));
        QCOMPARE(history.lastSuccess, Start);
        // told once, then again when it has gone on for as long again
        QVERIFY(!history.record(false, Start + 16 * Day));
        QVERIFY(!history.record(false, Start + 28 * Day));
        QVERIFY(history.record(false, Start + 29 * Day));
    }

    void test_fewChecksAreNotEnough()
    {
        // started twice, a month apart, offline both times
        CheckHistory history;
        QVERIFY(!history.record(false, Start));
        QVERIFY(!history.record(false, Start + 30 * Day));
        QVERIFY(history.record(false, Start + 31 * Day));
    }

    void test_manyChecksInShortTimeAreNotEnough()
    {
        CheckHistory history;
        for(int i = 0; i < 100; i++)
        {
            QVERIFY(!history.record(false, Start + i));
        }
    }

    void test_successEndsIt()
    {
        CheckHistory history;
        for(int day = 0; day < 14; day++)
        {
            QVERIFY(!history.record(false, Start + day * Day));
        }
        QVERIFY(!history.record(true, Start + 14 * Day));
        QCOMPARE(history.failingSince, qint64(0));
        QVERIFY(!history.record(false, Start + 15 * Day));
        QCOMPARE(history.failingSince, Start + 15 * Day);
        QCOMPARE(history.failedChecks, 1);
    }

    void test_neverSucceeded()
    {
        // an installation whose checks have never worked is told too
        CheckHistory history;
        for(int day = 0; day < 14; day++)
        {
            QVERIFY(!history.record(false, Start + day * Day));
        }
        QVERIFY(history.record(false, Start + 14 * Day));
        QCOMPARE(history.lastSuccess, qint64(0));
    }

    void test_clockWentBack()
    {
        CheckHistory history;
        history.record(false, Start + 100 * Day);
        history.record(false, Start + 101 * Day);
        // the wait starts over from the earlier time
        QVERIFY(!history.record(false, Start));
        QCOMPARE(history.failingSince, Start);
        QCOMPARE(history.failedChecks, 1);
        QVERIFY(!history.record(false, Start + 13 * Day));
        QVERIFY(history.record(false, Start + 14 * Day));
    }
};

QTEST_GUILESS_MAIN(UpdaterV2CheckHistoryTest)

#include "CheckHistory_test.moc"
