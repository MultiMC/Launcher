#include <QTest>
#include <QCryptographicHash>
#include <QTemporaryDir>
#include "TestUtil.h"

#include "meta/Index.h"
#include "meta/VersionList.h"
#include "meta/Version.h"
#include "FileSystem.h"
#include "Json.h"

namespace {
const QByteArray listJson =
    "{\"formatVersion\": 1, \"uid\": \"net.test\", \"versions\": ["
    "{\"version\": \"1.0\", \"releaseTime\": \"2026-11-01T00:00:00+00:00\", \"sha256\": \"" + QByteArray(64, '1') + "\"},"
    "{\"version\": \"2.0\", \"releaseTime\": \"2026-11-02T00:00:00+00:00\", \"sha256\": \"" + QByteArray(64, '2') + "\"}"
    "]}";

QByteArray sha256(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

QJsonObject object(const QByteArray &data)
{
    return Json::requireObject(Json::requireDocument(data));
}
}

class IndexTest : public QObject
{
    Q_OBJECT
private
slots:
    void test_hasUid_and_getList()
    {
        Meta::Index windex({std::make_shared<Meta::VersionList>("list1"), std::make_shared<Meta::VersionList>("list2"), std::make_shared<Meta::VersionList>("list3")});
        QVERIFY(windex.hasUid("list1"));
        QVERIFY(!windex.hasUid("asdf"));
        QVERIFY(windex.get("list2") != nullptr);
        QCOMPARE(windex.get("list2")->uid(), QString("list2"));
        QVERIFY(windex.get("adsf") != nullptr);
    }

    void test_merge()
    {
        Meta::Index windex({std::make_shared<Meta::VersionList>("list1"), std::make_shared<Meta::VersionList>("list2"), std::make_shared<Meta::VersionList>("list3")});
        QCOMPARE(windex.lists().size(), 3);
        windex.merge(std::shared_ptr<Meta::Index>(new Meta::Index({std::make_shared<Meta::VersionList>("list1"), std::make_shared<Meta::VersionList>("list2"), std::make_shared<Meta::VersionList>("list3")})));
        QCOMPARE(windex.lists().size(), 3);
        windex.merge(std::shared_ptr<Meta::Index>(new Meta::Index({std::make_shared<Meta::VersionList>("list4"), std::make_shared<Meta::VersionList>("list2"), std::make_shared<Meta::VersionList>("list5")})));
        QCOMPARE(windex.lists().size(), 5);
        windex.merge(std::shared_ptr<Meta::Index>(new Meta::Index({std::make_shared<Meta::VersionList>("list6")})));
        QCOMPARE(windex.lists().size(), 6);
    }

    void test_indexHashes()
    {
        Meta::Index index;
        auto early = index.get("net.test");
        index.parse(object("{\"formatVersion\": 1, \"packages\": [{\"uid\": \"net.test\", \"sha256\": \"" + QByteArray(64, 'a') + "\"}]}"));
        // the list handed out before the index loaded is the one that gets listed
        QCOMPARE(index.get("net.test"), early);
        QCOMPARE(index.lists().size(), 1);
        QCOMPARE(index.lists().first(), early);
        QCOMPARE(early->sha256(), QByteArray::fromHex(QByteArray(64, 'a')));

        index.parse(object("{\"formatVersion\": 1, \"packages\": [{\"uid\": \"net.test\", \"sha256\": \"" + QByteArray(64, 'b') + "\"}]}"));
        QCOMPARE(early->sha256(), QByteArray::fromHex(QByteArray(64, 'b')));
    }

    void test_indexRequiresHashes()
    {
        Meta::Index index;
        QVERIFY_EXCEPTION_THROWN(index.parse(object("{\"formatVersion\": 1, \"packages\": [{\"uid\": \"net.test\"}]}")), Json::JsonException);
        QVERIFY_EXCEPTION_THROWN(index.parse(object("{\"formatVersion\": 1, \"packages\": [{\"uid\": \"net.test\", \"sha256\": \"ab\"}]}")), Json::JsonException);
    }

    void test_versionHashes()
    {
        Meta::VersionList list("net.test");
        auto early = list.getVersion("1.0");
        list.parse(object(listJson));
        // the version handed out before the list loaded is the one in the list
        QCOMPARE(list.getVersion("1.0"), early);
        QCOMPARE(list.count(), 2);
        QVERIFY(list.versions().contains(early));
        QCOMPARE(early->sha256(), QByteArray::fromHex(QByteArray(64, '1')));
        QCOMPARE(list.getVersion("2.0")->sha256(), QByteArray::fromHex(QByteArray(64, '2')));

        // and stays the same object when the list loads again
        auto second = list.getVersion("2.0");
        list.parse(object(listJson));
        QCOMPARE(list.getVersion("2.0"), second);
        QVERIFY(list.versions().contains(second));
    }

    void test_current()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        auto previous = QDir::currentPath();
        QDir::setCurrent(dir.path());
        FS::write(FS::PathCombine(dir.path(), "meta", "net.test", "index.json"), listJson);

        Meta::Index index;
        auto list = index.get("net.test");
        QVERIFY(!list->isCurrent());
        list->load(Net::Mode::Offline);
        QVERIFY(list->isLoaded());
        // nothing says what's current yet
        QVERIFY(!list->isCurrent());

        list->setSha256(sha256(listJson));
        QVERIFY(list->isCurrent());

        // stale, but still loaded
        list->setSha256(sha256("something else"));
        QVERIFY(!list->isCurrent());
        QVERIFY(list->isLoaded());
        QCOMPARE(list->count(), 2);

        QDir::setCurrent(previous);
    }
};

QTEST_GUILESS_MAIN(IndexTest)

#include "Index_test.moc"
