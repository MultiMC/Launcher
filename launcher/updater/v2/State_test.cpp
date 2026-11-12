#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <QTemporaryDir>

#include "TestUtil.h"
#include "TestTrees.h"
#include "BuildConfig.h"
#include "updater/v2/State.h"

using namespace UpdaterV2;

class UpdaterV2StateTest : public QObject
{
    Q_OBJECT

    KeysDocument keys2()
    {
        KeysDocument keys;
        QString error;
        parseKeys(TestTrees::read("main", "keys/2.json"), 2, keys, error);
        return keys;
    }

    MirrorsDocument mirrors()
    {
        MirrorsDocument out;
        QString error;
        parseMirrors(TestTrees::read("main", "mirrors.json"), out, error);
        return out;
    }

private
slots:
    // the configuration this build was made with is usable, so a misconfigured build fails its tests
    void test_compiledIn()
    {
        if(!BuildConfig.UPDATER_ENABLED || BuildConfig.UPDATER_VERSION != 2)
        {
            QSKIP("updater v2 isn't used in this build");
        }
        State state(BuildConfig.UPDATER_KEYS_FLOOR, BuildConfig.UPDATER_ROOT_KEYS, BuildConfig.UPDATER_SERIAL_FLOOR);
        QVERIFY2(state.error().isEmpty(), qPrintable(state.error()));
        QVERIFY(!BuildConfig.UPDATER_VARIANT.isEmpty());
    }

    void test_floors()
    {
        State state(TestTrees::floor(), TestTrees::rootKeys(), 5);
        QVERIFY(state.error().isEmpty());
        QCOMPARE(state.keys().version, qint64(1));
        QCOMPARE(state.serial(), qint64(5));
        QVERIFY(!state.hasMirrors());
    }

    void test_pinnedKeysMustMatch()
    {
        auto roots = TestTrees::rootKeys();
        QVERIFY(!State(TestTrees::floor(), roots.mid(0, 1), 0).error().isEmpty());
        QVERIFY(!State(TestTrees::floor(), roots + QStringList(QString(64, 'a')), 0).error().isEmpty());
        QVERIFY(!State(TestTrees::floor(), QStringList() << roots[0] << roots[0], 0).error().isEmpty());
        QVERIFY(!State(TestTrees::floor(), QStringList(), 0).error().isEmpty());
        QVERIFY(!State(TestTrees::floor().left(50), roots, 0).error().isEmpty());
        QVERIFY(!State(QByteArray(), roots, 0).error().isEmpty());
        QVERIFY(State(TestTrees::floor(), QStringList() << roots[1] << roots[0], 0).error().isEmpty());
    }

    void test_neverLower()
    {
        State state(TestTrees::floor(), TestTrees::rootKeys(), 5);
        QVERIFY(!state.acceptSerial(4));
        QVERIFY(!state.acceptSerial(5));
        QVERIFY(state.acceptSerial(6));
        QCOMPARE(state.serial(), qint64(6));

        QVERIFY(state.acceptKeys(keys2(), TestTrees::read("main", "keys/2.json")));
        KeysDocument keys1;
        QString error;
        parseKeys(TestTrees::floor(), 1, keys1, error);
        QVERIFY(!state.acceptKeys(keys1, TestTrees::floor()));
        QCOMPARE(state.keys().version, qint64(2));

        auto list = mirrors();
        QVERIFY(state.acceptMirrors(list, TestTrees::read("main", "mirrors.json")));
        auto older = list;
        older.issued = older.issued.addDays(-1);
        QVERIFY(!state.acceptMirrors(older, "{}"));
        QCOMPARE(state.mirrors().groups, list.groups);
    }

    void test_saveAndLoad()
    {
        QTemporaryDir dir;
        auto path = dir.path() + "/updater/state.json";
        {
            State state(TestTrees::floor(), TestTrees::rootKeys(), 0);
            state.acceptSerial(7);
            state.acceptKeys(keys2(), TestTrees::read("main", "keys/2.json"));
            state.acceptMirrors(mirrors(), TestTrees::read("main", "mirrors.json"));
            QVERIFY(state.save(path));
        }
        State state(TestTrees::floor(), TestTrees::rootKeys(), 0);
        state.load(path);
        QCOMPARE(state.serial(), qint64(7));
        QCOMPARE(state.keys().version, qint64(2));
        QCOMPARE(state.keys().onlineKeys, keys2().onlineKeys);
        QVERIFY(state.hasMirrors());
        QCOMPARE(state.mirrors().groups, mirrors().groups);

        // the floors win over lower stored values
        State higher(TestTrees::floor(), TestTrees::rootKeys(), 9);
        higher.load(path);
        QCOMPARE(higher.serial(), qint64(9));
    }

    void test_brokenStateFallsBackToFloors_data()
    {
        QTest::addColumn<QByteArray>("content");
        QTest::newRow("garbage") << QByteArray("\x00\x01garbage", 9);
        QTest::newRow("not an object") << QByteArray("[1, 2]");
        QTest::newRow("other format") << QByteArray("{\"format\": 2, \"serial\": 100}");
        // these two have the right root keys: each broken part falls back on its own
        auto roots = QJsonDocument(QJsonArray::fromStringList(TestTrees::rootKeys())).toJson(QJsonDocument::Compact);
        QTest::newRow("broken keys") << QByteArray("{\"format\": 1, \"root_keys\": " + roots + ", \"keys\": \"{\\\"type\\\": \\\"keys\\\"}\"}");
        QTest::newRow("bad serial") << QByteArray("{\"format\": 1, \"root_keys\": " + roots + ", \"serial\": \"100\"}");
    }
    void test_brokenStateFallsBackToFloors()
    {
        QFETCH(QByteArray, content);
        QTemporaryDir dir;
        auto path = dir.path() + "/state.json";
        QFile file(path);
        QVERIFY(file.open(QFile::WriteOnly));
        file.write(content);
        file.close();
        State state(TestTrees::floor(), TestTrees::rootKeys(), 3);
        state.load(path);
        QCOMPARE(state.serial(), qint64(3));
        QCOMPARE(state.keys().version, qint64(1));
        QVERIFY(!state.hasMirrors());
    }

    void test_otherRootKeys()
    {
        // written by a build of the main tree, read by one pinned to the rotated tree's keys
        QTemporaryDir dir;
        auto path = dir.path() + "/state.json";
        {
            State state(TestTrees::floor(), TestTrees::rootKeys(), 0);
            state.acceptSerial(7);
            state.acceptKeys(keys2(), TestTrees::read("main", "keys/2.json"));
            state.acceptMirrors(mirrors(), TestTrees::read("main", "mirrors.json"));
            QVERIFY(state.save(path));
        }
        auto otherFloor = TestTrees::read("rotated", "keys/3.json");
        QStringList otherRoots;
        for(auto key: QJsonDocument::fromJson(otherFloor).object().value("root_keys").toArray())
        {
            otherRoots.append(key.toString());
        }
        State other(otherFloor, otherRoots, 2);
        QVERIFY2(other.error().isEmpty(), qPrintable(other.error()));
        other.load(path);
        QCOMPARE(other.serial(), qint64(2));
        QCOMPARE(other.keys().version, qint64(3));
        QVERIFY(!other.hasMirrors());

        // a state without root keys, as written before they were stored, is ignored too
        QFile file(path);
        QVERIFY(file.open(QFile::WriteOnly));
        file.write("{\"format\": 1, \"serial\": 100}");
        file.close();
        State same(TestTrees::floor(), TestTrees::rootKeys(), 3);
        same.load(path);
        QCOMPARE(same.serial(), qint64(3));
    }

    void test_missingState()
    {
        QTemporaryDir dir;
        State state(TestTrees::floor(), TestTrees::rootKeys(), 3);
        state.load(dir.path() + "/nothing.json");
        QCOMPARE(state.serial(), qint64(3));
    }
};

QTEST_GUILESS_MAIN(UpdaterV2StateTest)

#include "State_test.moc"
