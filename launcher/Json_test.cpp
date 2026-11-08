#include <QTest>
#include "TestUtil.h"

#include "Json.h"

class JsonTest : public QObject
{
    Q_OBJECT
private
slots:
    void test_byteArray()
    {
        QCOMPARE(Json::requireValueByteArray(QJsonValue("00ff")), QByteArray::fromHex("00ff"));
    }

    void test_byteArrayNotAString()
    {
        QVERIFY_EXCEPTION_THROWN(Json::requireValueByteArray(QJsonValue(5)), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueByteArray(QJsonValue(QJsonValue::Null)), Json::JsonException);
        QCOMPARE(Json::ensureValueByteArray(QJsonValue(5), QByteArray("default")), QByteArray("default"));
    }

    void test_url()
    {
        QCOMPARE(Json::requireValueUrl(QJsonValue("https://example.com/a")), QUrl("https://example.com/a"));
    }

    void test_urlNotAString()
    {
        QVERIFY_EXCEPTION_THROWN(Json::requireValueUrl(QJsonValue(5)), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueUrl(QJsonValue(QJsonValue::Null)), Json::JsonException);
    }

    void test_urlEmpty()
    {
        QVERIFY_EXCEPTION_THROWN(Json::requireValueUrl(QJsonValue("")), Json::JsonException);
    }

    // a missing, null or empty URL is the default when the URL is optional
    void test_ensureUrl()
    {
        QJsonObject object;
        object.insert("null", QJsonValue());
        object.insert("empty", QString());
        object.insert("number", 5);
        QCOMPARE(Json::ensureUrl(object, "missing"), QUrl());
        QCOMPARE(Json::ensureUrl(object, "null"), QUrl());
        QCOMPARE(Json::ensureUrl(object, "empty"), QUrl());
        QCOMPARE(Json::ensureUrl(object, "number"), QUrl());
    }

    void test_intOutOfRange()
    {
        QCOMPARE(Json::requireValueInteger(QJsonValue(2147483647.0)), 2147483647);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueInteger(QJsonValue(2147483648.0)), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueInteger(QJsonValue(-2147483649.0)), Json::JsonException);
    }

    void test_shortDocument()
    {
        QVERIFY_EXCEPTION_THROWN(Json::requireDocument(QByteArray("q")), Json::JsonException);
    }

    void test_textDocument()
    {
        auto doc = Json::requireTextDocument("{\"a\": \"\xc3\xa9\"}");
        QCOMPARE(Json::requireString(doc.object(), "a"), QString::fromUtf8("\xc3\xa9"));
    }

    void test_textDocumentRefuses_data()
    {
        QTest::addColumn<QByteArray>("data");
        QTest::newRow("binary") << Json::toBinary(QJsonObject());
        QTest::newRow("byte order mark") << QByteArray("\xEF\xBB\xBF{}");
        QTest::newRow("invalid UTF-8") << QByteArray("{\"a\": \"\xff\"}");
        QTest::newRow("trailing data") << QByteArray("{} {}");
        QTest::newRow("empty") << QByteArray();
    }
    void test_textDocumentRefuses()
    {
        QFETCH(QByteArray, data);
        QVERIFY_EXCEPTION_THROWN(Json::requireTextDocument(data), Json::JsonException);
    }

    void test_exactKeys()
    {
        auto object = Json::requireTextDocument("{\"a\": 1, \"b\": null}").object();
        Json::requireExactKeys(object, {"a", "b"});
        Json::requireExactKeys(object, {"b", "a"});
        QVERIFY_EXCEPTION_THROWN(Json::requireExactKeys(object, {"a"}), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireExactKeys(object, {"a", "b", "c"}), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireExactKeys(object, {"a", "c"}), Json::JsonException);
    }

    void test_keys()
    {
        auto object = Json::requireTextDocument("{\"a\": 1, \"b\": null}").object();
        Json::requireKeys(object, {"a", "b"});
        Json::requireKeys(object, {"a"});
        Json::requireKeys(object, {});
        QVERIFY_EXCEPTION_THROWN(Json::requireKeys(object, {"a", "c"}), Json::JsonException);
    }

    void test_boundedInteger()
    {
        QCOMPARE(Json::requireValueBoundedInteger(QJsonValue(0), 0, 10), qint64(0));
        QCOMPARE(Json::requireValueBoundedInteger(QJsonValue(10), 0, 10), qint64(10));
        QCOMPARE(Json::requireValueBoundedInteger(QJsonValue(double(Json::MaxExactInteger)), 0, Json::MaxExactInteger), Json::MaxExactInteger);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueBoundedInteger(QJsonValue(-1), 0, 10), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueBoundedInteger(QJsonValue(11), 0, 10), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueBoundedInteger(QJsonValue(1.5), 0, 10), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueBoundedInteger(QJsonValue("1"), 0, 10), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueBoundedInteger(QJsonValue(QJsonValue::Null), 0, 10), Json::JsonException);
        // 2^53 is past what a JSON number holds exactly, whatever the maximum asked for
        QVERIFY_EXCEPTION_THROWN(Json::requireValueBoundedInteger(QJsonValue(9007199254740992.0), 0, std::numeric_limits<qint64>::max()), Json::JsonException);
    }

    void test_boundedIntegerKey()
    {
        auto object = Json::requireTextDocument("{\"n\": 5}").object();
        QCOMPARE(Json::requireBoundedInteger(object, "n", 1, 5), qint64(5));
        QVERIFY_EXCEPTION_THROWN(Json::requireBoundedInteger(object, "missing", 1, 5), Json::JsonException);
    }

    void test_hex()
    {
        QCOMPARE(Json::requireValueHex(QJsonValue("00ff"), 2), QByteArray::fromHex("00ff"));
        QVERIFY_EXCEPTION_THROWN(Json::requireValueHex(QJsonValue("00FF"), 2), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueHex(QJsonValue("00f"), 2), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueHex(QJsonValue("00ff00"), 2), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueHex(QJsonValue("00 f"), 2), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueHex(QJsonValue("00fg"), 2), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueHex(QJsonValue(5), 2), Json::JsonException);
    }

    void test_utcTime()
    {
        auto time = Json::requireValueUtcTime(QJsonValue("2026-11-01T12:34:56Z"));
        QCOMPARE(time, QDateTime(QDate(2026, 11, 1), QTime(12, 34, 56), Qt::UTC));
        QCOMPARE(time.timeSpec(), Qt::UTC);
    }

    void test_utcTimeRefuses_data()
    {
        QTest::addColumn<QString>("time");
        QTest::newRow("date only") << "2026-11-01";
        QTest::newRow("no zone") << "2026-11-01T12:34:56";
        QTest::newRow("offset") << "2026-11-01T12:34:56+00:00";
        QTest::newRow("fraction") << "2026-11-01T12:34:56.000Z";
        QTest::newRow("no leading zero") << "2026-11-1T12:34:56Z";
        QTest::newRow("lowercase") << "2026-11-01t12:34:56z";
        QTest::newRow("no such day") << "2026-02-30T12:34:56Z";
        QTest::newRow("no such hour") << "2026-11-01T24:00:00Z";
        QTest::newRow("empty") << "";
    }
    void test_utcTimeRefuses()
    {
        QFETCH(QString, time);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueUtcTime(QJsonValue(time)), Json::JsonException);
    }

    void test_matching()
    {
        const QString version = "(0|[1-9][0-9]*)(\\.(0|[1-9][0-9]*))*";
        QCOMPARE(Json::requireValueMatching(QJsonValue("10.13"), version), QString("10.13"));
        QVERIFY_EXCEPTION_THROWN(Json::requireValueMatching(QJsonValue("10.013"), version), Json::JsonException);
        // the whole string has to match, not just a part of it
        QVERIFY_EXCEPTION_THROWN(Json::requireValueMatching(QJsonValue("x10.13"), version), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueMatching(QJsonValue("10.13\n"), version), Json::JsonException);
        // alternatives in the pattern don't escape the anchors
        QVERIFY_EXCEPTION_THROWN(Json::requireValueMatching(QJsonValue("ab"), "a|b"), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueMatching(QJsonValue(5), version), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(Json::requireValueMatching(QJsonValue("a"), "("), Json::JsonException);
    }
};

QTEST_GUILESS_MAIN(JsonTest)

#include "Json_test.moc"
