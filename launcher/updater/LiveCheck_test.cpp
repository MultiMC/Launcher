#include <QTest>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QTimer>

#include "updater/LiveCheck.h"

class LiveCheckTest : public QObject
{
    Q_OBJECT

    static void write(const QString &path, const QByteArray &data)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(data);
    }

private
slots:
    // a slow start: the key shows up well after the first look, the wait goes on
    void test_lateKey()
    {
        QTemporaryDir dir;
        auto path = dir.path() + "/live.check";
        QTimer::singleShot(800, [path]() { write(path, "the-key"); });
        QString asked;
        bool started = LiveCheck::waitFor(path, 5000, [&asked](const QString &key) {
            asked = key;
            return true;
        });
        QVERIFY(started);
        QCOMPARE(asked, QString("the-key"));
    }

    // created but not written yet: wait for the key instead of calling it dead
    void test_emptyFileThenKey()
    {
        QTemporaryDir dir;
        auto path = dir.path() + "/live.check";
        write(path, "");
        QTimer::singleShot(600, [path]() { write(path, "the-key"); });
        int asked = 0;
        QVERIFY(LiveCheck::waitFor(path, 5000, [&asked](const QString &) {
            asked++;
            return true;
        }));
        QCOMPARE(asked, 1);
    }

    void test_noKeyTimesOut()
    {
        QTemporaryDir dir;
        auto path = dir.path() + "/live.check";
        QElapsedTimer elapsed;
        elapsed.start();
        bool asked = false;
        QVERIFY(!LiveCheck::waitFor(path, 700, [&asked](const QString &) {
            asked = true;
            return true;
        }));
        QVERIFY(!asked);
        QVERIFY(elapsed.elapsed() >= 700);
        QVERIFY(elapsed.elapsed() < 3000);
    }

    // the key is there but nothing answers for it: fail without waiting out the timeout
    void test_diedFailsRightAway()
    {
        QTemporaryDir dir;
        auto path = dir.path() + "/live.check";
        write(path, "the-key");
        QElapsedTimer elapsed;
        elapsed.start();
        QVERIFY(!LiveCheck::waitFor(path, 10000, [](const QString &) { return false; }));
        QVERIFY(elapsed.elapsed() < 3000);
    }
};

QTEST_GUILESS_MAIN(LiveCheckTest)

#include "LiveCheck_test.moc"
