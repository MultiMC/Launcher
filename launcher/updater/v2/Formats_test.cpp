#include <QTest>

#include "TestUtil.h"
#include "TestTrees.h"
#include "updater/v2/Formats.h"

using namespace UpdaterV2;

namespace {

// a small valid manifest; the tests change one thing at a time
const char *manifest =
    "{\n"
    "  \"type\": \"build\",\n"
    "  \"format\": 1,\n"
    "  \"build\": 5,\n"
    "  \"commit\": \"0123456789abcdef0123456789abcdef01234567\",\n"
    "  \"built\": \"2026-11-01T11:42:07Z\",\n"
    "  \"variants\": {\n"
    "    \"macos-x86_64-qt5.15\": {\n"
    "      \"name\": \"macOS, Qt 5.15\",\n"
    "      \"supersedes\": [\"macos-x86_64-qt5.6\"],\n"
    "      \"requires\": {\"os\": \"macos\", \"arch\": \"x86_64\", \"os_version\": \"10.13\"},\n"
    "      \"files\": [\n"
    "        {\"path\": \"A.app/Contents/Frameworks/Q.framework/Versions/5/Q\", \"sha256\": \"9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08\", \"size\": 4, \"executable\": false},\n"
    "        {\"path\": \"A.app/Contents/MacOS/A\", \"sha256\": \"9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08\", \"size\": 4, \"executable\": true}\n"
    "      ],\n"
    "      \"links\": [\n"
    "        {\"path\": \"A.app/Contents/Frameworks/Q.framework/Q\", \"target\": \"Versions/Current/Q\"},\n"
    "        {\"path\": \"A.app/Contents/Frameworks/Q.framework/Versions/Current\", \"target\": \"5\"}\n"
    "      ]\n"
    "    },\n"
    "    \"linux-x86_64-qt5\": {\n"
    "      \"name\": \"Linux, 64-bit\",\n"
    "      \"supersedes\": [],\n"
    "      \"requires\": {\"os\": \"linux\", \"arch\": \"x86_64\", \"glibc\": \"2.19\", \"glibcxx\": \"3.4.21\", \"qt\": \"5.6\"},\n"
    "      \"files\": [\n"
    "        {\"path\": \"bin/MultiMC\", \"sha256\": \"9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08\", \"size\": 4, \"executable\": true}\n"
    "      ],\n"
    "      \"links\": []\n"
    "    }\n"
    "  }\n"
    "}\n";

const char *keysDocument =
    "{\"type\": \"keys\", \"format\": 1, \"version\": 3, \"expires\": \"2028-11-01T00:00:00Z\",\n"
    "  \"root_keys\": [\"1111111111111111111111111111111111111111111111111111111111111111\"],\n"
    "  \"online_keys\": [\"2222222222222222222222222222222222222222222222222222222222222222\"]}\n";

const char *tagsDocument =
    "{\"type\": \"tags\", \"format\": 1, \"serial\": 1042, \"issued\": \"2026-11-01T12:00:00Z\",\n"
    "  \"tags\": {\"latest\": 4312, \"testing\": 4305, \"stable\": 4290}, \"withdrawn\": [4311]}\n";

const char *mirrorsDocument =
    "{\"type\": \"mirrors\", \"format\": 1, \"issued\": \"2026-11-01T12:00:00Z\",\n"
    "  \"mirrors\": [[\"https://files.multimc.org/\"], [\"https://bucket.example/\", \"https://other.example/x/\"]]}\n";

QByteArray changed(const char *document, const QString &from, const QString &to)
{
    auto data = QByteArray(document);
    auto out = data;
    out.replace(from.toUtf8(), to.toUtf8());
    if(out == data)
    {
        qFatal("the change doesn't apply: %s", qPrintable(from));
    }
    return out;
}

}

class UpdaterV2FormatsTest : public QObject
{
    Q_OBJECT

private
slots:
    // --- the trees, as the publisher's code writes them ---

    void test_mainTree()
    {
        KeysDocument keys1, keys2;
        QString error;
        QVERIFY2(parseKeys(TestTrees::read("main", "keys/1.json"), 1, keys1, error), qPrintable(error));
        QVERIFY2(parseKeys(TestTrees::read("main", "keys/2.json"), 2, keys2, error), qPrintable(error));
        QCOMPARE(keys1.rootKeys, TestTrees::rootKeys());
        QCOMPARE(keys2.rootKeys, keys1.rootKeys);
        QCOMPARE(keys2.onlineKeys.size(), 1);
        QCOMPARE(keys2.expires, QDateTime(QDate(2028, 11, 1), QTime(0, 0), Qt::UTC));

        TagsDocument tags;
        QVERIFY2(parseTags(TestTrees::read("main", "tags.json"), tags, error), qPrintable(error));
        QCOMPARE(tags.serial, qint64(7));
        QCOMPARE(tags.latest, qint64(12));
        QCOMPARE(tags.testing, qint64(11));
        QCOMPARE(tags.stable, qint64(10));
        QCOMPARE(tags.withdrawn, QList<qint64>() << 9);

        MirrorsDocument mirrors;
        QVERIFY2(parseMirrors(TestTrees::read("main", "mirrors.json"), mirrors, error), qPrintable(error));
        QCOMPARE(mirrors.groups.size(), 2);
        QCOMPARE(mirrors.groups[0], QStringList() << "https://files.invalid/");
        QCOMPARE(mirrors.groups[1].size(), 2);

        BuildManifest build;
        QVERIFY2(parseBuild(TestTrees::read("main", "builds/11.json"), 11, build, error), qPrintable(error));
        QCOMPARE(QStringList(build.variants.keys()), QStringList() << "linux-x86_64-qt5" << "macos-x86_64-qt5.15" << "macos-x86_64-qt5.6" << "windows-x86-qt5.6");
        auto mac = build.variants.value("macos-x86_64-qt5.15");
        QCOMPARE(mac.supersedes, QStringList() << "macos-x86_64-qt5.6");
        QCOMPARE(mac.requirements.osVersion, QString("10.13"));
        QCOMPARE(mac.links.size(), 2);
        QCOMPARE(mac.links[1].target, QString("5"));
        auto linuxVariant = build.variants.value("linux-x86_64-qt5");
        QCOMPARE(linuxVariant.requirements.glibcxx, QString("3.4.21"));
        QCOMPARE(linuxVariant.files[0].path, QString("MultiMC"));
        QVERIFY(linuxVariant.files[0].executable);
        QCOMPARE(linuxVariant.files[0].sha256.size(), 64);
    }

    void test_signatures()
    {
        KeysDocument keys1, keys2;
        QString error;
        parseKeys(TestTrees::read("main", "keys/1.json"), 1, keys1, error);
        parseKeys(TestTrees::read("main", "keys/2.json"), 2, keys2, error);
        auto tags = TestTrees::read("main", "tags.json");
        auto signature = TestTrees::read("main", "tags.json.sig");
        QVERIFY2(checkSignatureFile(tags, signature, keys2.onlineKeys, error), qPrintable(error));

        // the online key before the rotation doesn't sign it any more
        QVERIFY(!checkSignatureFile(tags, signature, keys1.onlineKeys, error));
        QCOMPARE(error, QString("no signature by a trusted key"));

        // keys/1.json is signed by both root keys, keys/2.json by the first one
        QVERIFY(checkSignatureFile(TestTrees::read("main", "keys/1.json"), TestTrees::read("main", "keys/1.json.sig"), keys1.rootKeys, error));
        QVERIFY(checkSignatureFile(TestTrees::read("main", "keys/2.json"), TestTrees::read("main", "keys/2.json.sig"), keys1.rootKeys, error));
        QVERIFY(checkSignatureFile(TestTrees::read("main", "keys/2.json"), TestTrees::read("main", "keys/2.json.sig"), QStringList() << keys1.rootKeys[0], error));
        QVERIFY(!checkSignatureFile(TestTrees::read("main", "keys/2.json"), TestTrees::read("main", "keys/2.json.sig"), QStringList() << keys1.rootKeys[1], error));

        // changed after signing
        QVERIFY(!checkSignatureFile(TestTrees::read("broken", "tags.json"), TestTrees::read("broken", "tags.json.sig"), keys2.onlineKeys, error));
        QVERIFY(error.contains("doesn't verify"));
        auto extended = tags + " ";
        QVERIFY(!checkSignatureFile(extended, signature, keys2.onlineKeys, error));
    }

    void test_signatureFileFormat_data()
    {
        QTest::addColumn<QByteArray>("signature");
        auto line = TestTrees::read("main", "tags.json.sig");
        auto other = TestTrees::read("main", "keys/1.json.sig").split('\n')[0] + "\n";
        QTest::newRow("empty") << QByteArray();
        QTest::newRow("no final newline") << line.left(line.size() - 1);
        QTest::newRow("CRLF") << QByteArray(line).replace("\n", "\r\n");
        QTest::newRow("blank line") << other + "\n" + line;
        QTest::newRow("same key twice") << line + line;
        QTest::newRow("uppercase") << line.toUpper();
        QTest::newRow("two spaces") << QByteArray(line).replace(" ", "  ");
        QByteArray nine;
        for(int i = 0; i < 8; i++)
        {
            nine += QByteArray(other).replace(other.left(2), QByteArray::number(10 + i));
        }
        QTest::newRow("nine lines") << nine + line;
    }
    void test_signatureFileFormat()
    {
        QFETCH(QByteArray, signature);
        KeysDocument keys2;
        QString error;
        parseKeys(TestTrees::read("main", "keys/2.json"), 2, keys2, error);
        QVERIFY(!checkSignatureFile(TestTrees::read("main", "tags.json"), signature, keys2.onlineKeys, error));
    }

    void test_untrustedLinesIgnored()
    {
        KeysDocument keys2;
        QString error;
        parseKeys(TestTrees::read("main", "keys/2.json"), 2, keys2, error);
        // a line by another key that doesn't verify is ignored...
        auto line = TestTrees::read("main", "tags.json.sig");
        auto stranger = QByteArray(64, '3') + " " + QByteArray(128, '4') + "\n";
        QVERIFY2(checkSignatureFile(TestTrees::read("main", "tags.json"), stranger + line, keys2.onlineKeys, error), qPrintable(error));
        // ...one by a trusted key makes the whole file invalid
        QVERIFY(!checkSignatureFile(TestTrees::read("main", "tags.json"), stranger + line, keys2.onlineKeys + QStringList(QString(64, '3')), error));
    }

    // --- strictness ---

    void test_validDocuments()
    {
        QString error;
        KeysDocument keys;
        TagsDocument tags;
        MirrorsDocument mirrors;
        BuildManifest build;
        QVERIFY2(parseKeys(keysDocument, 3, keys, error), qPrintable(error));
        QVERIFY2(parseTags(tagsDocument, tags, error), qPrintable(error));
        QVERIFY2(parseMirrors(mirrorsDocument, mirrors, error), qPrintable(error));
        QVERIFY2(parseBuild(manifest, 5, build, error), qPrintable(error));
        QCOMPARE(tags.withdrawn, QList<qint64>() << 4311);
        QCOMPARE(build.variants.size(), 2);
    }

    // fields added later in the same format are ignored, so additions don't strand Launchers
    void test_unknownKeysIgnored()
    {
        QString error;
        TagsDocument tags;
        auto tagsData = changed(changed(tagsDocument, "\"withdrawn\"", "\"extra\": {\"x\": [1]}, \"withdrawn\""), "\"stable\": 4290", "\"stable\": 4290, \"beta\": 4300");
        QVERIFY2(parseTags(tagsData, tags, error), qPrintable(error));
        QCOMPARE(tags.stable, qint64(4290));

        KeysDocument keys;
        QVERIFY2(parseKeys(changed(keysDocument, "\"version\"", "\"note\": \"x\", \"version\""), 3, keys, error), qPrintable(error));
        MirrorsDocument mirrors;
        QVERIFY2(parseMirrors(changed(mirrorsDocument, "\"issued\"", "\"weights\": [1, 2], \"issued\""), mirrors, error), qPrintable(error));

        BuildManifest build;
        auto buildData = changed(manifest, "\"commit\"", "\"notes\": \"x\", \"commit\"");
        buildData.replace("\"supersedes\": [],", "\"supersedes\": [], \"icon\": \"x\",");
        buildData.replace("\"executable\": true}", "\"executable\": true, \"mode\": 493}");
        buildData.replace("\"target\": \"5\"}", "\"target\": \"5\", \"kind\": \"dir\"}");
        QVERIFY2(parseBuild(buildData, 5, build, error), qPrintable(error));
        QCOMPARE(build.variants.value("linux-x86_64-qt5").name, QString("Linux, 64-bit"));
        QVERIFY(build.variants.value("linux-x86_64-qt5").requirements.unknown.isEmpty());
    }

    // a requirement or system this Launcher doesn't know is kept, and never counts as met
    void test_unknownRequirements()
    {
        QString error;
        BuildManifest build;
        auto data = changed(manifest, "\"qt\": \"5.6\"", "\"qt\": \"5.6\", \"gpu\": \"vulkan\", \"os_version\": \"1\"");
        QVERIFY2(parseBuild(data, 5, build, error), qPrintable(error));
        auto linuxVariant = build.variants.value("linux-x86_64-qt5");
        QCOMPARE(linuxVariant.requirements.unknown, QStringList() << "gpu" << "os_version");

        data = changed(manifest, "\"os\": \"linux\", \"arch\": \"x86_64\"", "\"os\": \"freebsd\", \"arch\": \"riscv64\"");
        data.replace("\"linux-x86_64-qt5\"", "\"freebsd-riscv64-qt5\"");
        QVERIFY2(parseBuild(data, 5, build, error), qPrintable(error));
        auto other = build.variants.value("freebsd-riscv64-qt5");
        QCOMPARE(other.requirements.os, QString("freebsd"));
        QCOMPARE(other.requirements.arch, QString("riscv64"));
    }

    void test_invalidTags_data()
    {
        QTest::addColumn<QByteArray>("data");
        QTest::newRow("missing field") << changed(tagsDocument, "\"issued\": \"2026-11-01T12:00:00Z\",", "");
        QTest::newRow("format 2") << changed(tagsDocument, "\"format\": 1", "\"format\": 2");
        QTest::newRow("format as a string") << changed(tagsDocument, "\"format\": 1", "\"format\": \"1\"");
        QTest::newRow("other type") << changed(tagsDocument, "\"type\": \"tags\"", "\"type\": \"mirrors\"");
        QTest::newRow("fraction") << changed(tagsDocument, "1042", "1042.5");
        QTest::newRow("serial 0") << changed(tagsDocument, "1042", "0");
        QTest::newRow("negative") << changed(tagsDocument, "4290", "-4290");
        QTest::newRow("above 2^53") << changed(tagsDocument, "1042", "9007199254740992");
        QTest::newRow("number as a string") << changed(tagsDocument, "1042", "\"1042\"");
        QTest::newRow("null") << changed(tagsDocument, "[4311]", "null");
        QTest::newRow("true as a number") << changed(tagsDocument, "1042", "true");
        QTest::newRow("stable above testing") << changed(tagsDocument, "4290", "4306");
        QTest::newRow("withdrawn tagged") << changed(tagsDocument, "[4311]", "[4305]");
        QTest::newRow("withdrawn not ascending") << changed(tagsDocument, "[4311]", "[4311, 4300]");
        QTest::newRow("withdrawn twice") << changed(tagsDocument, "[4311]", "[4311, 4311]");
        QTest::newRow("bad time") << changed(tagsDocument, "2026-11-01T12:00:00Z", "2026-02-30T12:00:00Z");
        QTest::newRow("time with offset") << changed(tagsDocument, "2026-11-01T12:00:00Z", "2026-11-01T12:00:00+01:00");
        QTest::newRow("time with a space") << changed(tagsDocument, "2026-11-01T12:00:00Z", "2026-11-01 12:00:00Z");
        QTest::newRow("byte order mark") << QByteArray("\xef\xbb\xbf") + tagsDocument;
        QTest::newRow("array") << QByteArray("[]");
        QTest::newRow("not JSON") << QByteArray(tagsDocument).left(40);
        QTest::newRow("too large") << changed(tagsDocument, "\"withdrawn\"", "\"pad\": \"" + QString(MaxDocumentSize, 'x') + "\", \"withdrawn\"");
    }
    void test_invalidTags()
    {
        QFETCH(QByteArray, data);
        TagsDocument tags;
        QString error;
        QVERIFY(!parseTags(data, tags, error));
        QVERIFY(!error.isEmpty());
    }

    void test_notUtf8()
    {
        TagsDocument tags;
        QString error;
        QVERIFY(!parseTags(QByteArray(tagsDocument).replace("12:00:00Z", "12:00:00\xff"), tags, error));
        QVERIFY2(error.contains("JSON"), qPrintable(error));
    }

    void test_invalidKeys_data()
    {
        QTest::addColumn<QByteArray>("data");
        QTest::addColumn<qint64>("version");
        QTest::newRow("another version") << QByteArray(keysDocument) << qint64(4);
        QTest::newRow("version 0") << changed(keysDocument, "\"version\": 3", "\"version\": 0") << qint64(0);
        QTest::newRow("both lists") << changed(keysDocument, "2222", "1111") << qint64(3);
        QTest::newRow("twice") << changed(keysDocument, "[\"1111111111111111111111111111111111111111111111111111111111111111\"]",
                                          "[\"1111111111111111111111111111111111111111111111111111111111111111\", \"1111111111111111111111111111111111111111111111111111111111111111\"]") << qint64(3);
        QTest::newRow("uppercase") << changed(keysDocument, "2222222222222222222222222222222222222222222222222222222222222222",
                                              "AAAA222222222222222222222222222222222222222222222222222222222222") << qint64(3);
        QTest::newRow("short key") << changed(keysDocument, "2222222222222222222222222222222222222222222222222222222222222222", "2222") << qint64(3);
        QTest::newRow("no online keys") << changed(keysDocument, "[\"2222222222222222222222222222222222222222222222222222222222222222\"]", "[]") << qint64(3);
        QString five;
        for(int i = 3; i < 8; i++)
        {
            five += QString("%1\"%2\"").arg(i == 3 ? "" : ", ").arg(QString(64, QChar('0' + i)));
        }
        QTest::newRow("five keys") << changed(keysDocument, "[\"2222222222222222222222222222222222222222222222222222222222222222\"]", "[" + five + "]") << qint64(3);
    }
    void test_invalidKeys()
    {
        QFETCH(QByteArray, data);
        QFETCH(qint64, version);
        KeysDocument keys;
        QString error;
        QVERIFY(!parseKeys(data, version, keys, error));
    }

    void test_invalidMirrors_data()
    {
        QTest::addColumn<QByteArray>("data");
        QTest::newRow("http") << changed(mirrorsDocument, "https://files", "http://files");
        QTest::newRow("no final slash") << changed(mirrorsDocument, "multimc.org/\"]", "multimc.org\"]");
        QTest::newRow("user info") << changed(mirrorsDocument, "https://files", "https://user@files");
        QTest::newRow("query") << changed(mirrorsDocument, "multimc.org/\"]", "multimc.org/?a=/\"]");
        QTest::newRow("fragment") << changed(mirrorsDocument, "example/x/", "example/#x/");
        QTest::newRow("no host") << changed(mirrorsDocument, "https://files.multimc.org/", "https:///");
        QTest::newRow("space") << changed(mirrorsDocument, "files.multimc", "files multimc");
        QTest::newRow("twice") << changed(mirrorsDocument, "https://other.example/x/", "https://files.multimc.org/");
        QTest::newRow("empty group") << changed(mirrorsDocument, "[[\"https://files.multimc.org/\"], ", "[[], ");
        QTest::newRow("no groups") << changed(mirrorsDocument, "[[\"https://files.multimc.org/\"], [\"https://bucket.example/\", \"https://other.example/x/\"]]", "[]");
        QString many;
        for(int i = 0; i < 17; i++)
        {
            many += QString("%1[\"https://m%2.example/\"]").arg(i ? ", " : "").arg(i);
        }
        QTest::newRow("seventeen") << changed(mirrorsDocument, "[[\"https://files.multimc.org/\"], [\"https://bucket.example/\", \"https://other.example/x/\"]]", "[" + many + "]");
    }
    void test_invalidMirrors()
    {
        QFETCH(QByteArray, data);
        MirrorsDocument mirrors;
        QString error;
        QVERIFY(!parseMirrors(data, mirrors, error));
    }

    // the build as a whole: nothing in it can be used
    void test_invalidBuild_data()
    {
        QTest::addColumn<QByteArray>("data");
        QTest::addColumn<QString>("message");
        QTest::newRow("another build") << changed(manifest, "\"build\": 5", "\"build\": 6") << "build is 6";
        QTest::newRow("short commit") << changed(manifest, "01234567\"", "\"") << "commit";
        QTest::newRow("no variants") << changed(manifest, "\"variants\": {", "\"variants\": {}, \"x\": {") << "variants";
        QTest::newRow("superseded twice") << changed(manifest, "\"supersedes\": [],", "\"supersedes\": [\"macos-x86_64-qt5.6\"],") << "both";
        QTest::newRow("loop") << changed(manifest, "\"supersedes\": [],", "\"supersedes\": [\"macos-x86_64-qt5.15\"],").replace("[\"macos-x86_64-qt5.6\"]", "[\"linux-x86_64-qt5\"]") << "loops";
    }
    void test_invalidBuild()
    {
        QFETCH(QByteArray, data);
        QFETCH(QString, message);
        BuildManifest build;
        QString error;
        QVERIFY(!parseBuild(data, 5, build, error));
        QVERIFY2(error.contains(message), qPrintable(error));
    }

    // one variant this Launcher can't read is left out, with why; the build and its other
    // variants stay usable, so a later format can't cut Launchers off from updates
    void test_unreadableVariant_data()
    {
        QTest::addColumn<QByteArray>("data");
        QTest::addColumn<QString>("message");
        QTest::newRow("bad variant ID") << changed(manifest, "\"linux-x86_64-qt5\"", "\"linux-x86_64\"") << "variant ID";
        QTest::newRow("ID doesn't match") << changed(manifest, "\"linux-x86_64-qt5\"", "\"linux-x86-qt5\"") << "doesn't match";
        QTest::newRow("bad os") << changed(manifest, "\"os\": \"linux\"", "\"os\": \"Linux\"") << "os";
        QTest::newRow("bad arch") << changed(manifest, "\"arch\": \"x86_64\", \"glibc\"", "\"arch\": \"x86-64\", \"glibc\"") << "arch";
        QTest::newRow("missing requirement") << changed(manifest, "\"glibc\": \"2.19\", ", "") << "'glibc'";
        QTest::newRow("no name") << changed(manifest, "\"name\": \"Linux, 64-bit\",\n", "") << "'name'";
        QTest::newRow("empty name") << changed(manifest, "Linux, 64-bit", "") << "name";
        QTest::newRow("name with a leading space") << changed(manifest, "Linux, 64-bit", " Linux") << "name";
        QTest::newRow("name not ASCII") << changed(manifest, "Linux, 64-bit", "Linux, 64\xe2\x80\x91" "bit") << "name";
        QTest::newRow("long name") << changed(manifest, "Linux, 64-bit", QString(65, 'x')) << "name";
        QTest::newRow("leading zero") << changed(manifest, "\"3.4.21\"", "\"3.04.21\"") << "glibcxx";
        QTest::newRow("no files") << changed(manifest, "\"files\": [\n        {\"path\": \"bin/MultiMC\", \"sha256\": \"9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08\", \"size\": 4, \"executable\": true}\n      ]", "\"files\": []") << "files";
        QTest::newRow("dot dot") << changed(manifest, "bin/MultiMC", "bin/../MultiMC") << "path";
        QTest::newRow("dot") << changed(manifest, "bin/MultiMC", "./MultiMC") << "path";
        QTest::newRow("leading slash") << changed(manifest, "bin/MultiMC", "/bin/MultiMC") << "path";
        QTest::newRow("trailing slash") << changed(manifest, "bin/MultiMC", "bin/MultiMC/") << "path";
        QTest::newRow("backslash") << changed(manifest, "bin/MultiMC", "bin\\\\MultiMC") << "path";
        QTest::newRow("colon") << changed(manifest, "bin/MultiMC", "C:/MultiMC") << "path";
        QTest::newRow("not ASCII") << changed(manifest, "bin/MultiMC", "bin/M\xc3\xbcltiMC") << "path";
        QTest::newRow("executable as a number") << changed(manifest, "\"executable\": true}\n      ],\n      \"links\": []", "\"executable\": 1}\n      ],\n      \"links\": []") << "executable";
        QTest::newRow("unsorted") << changed(manifest, "A.app/Contents/MacOS/A", "A.app/Contents/Frameworks/A") << "sorted";
        QTest::newRow("case") << changed(manifest, "A.app/Contents/MacOS/A", "A.app/contents/MacOS/A") << "case";
        QTest::newRow("file is a directory") << changed(manifest, "A.app/Contents/Frameworks/Q.framework/Versions/5/Q\", \"sha256\"", "A.app/Contents/Frameworks\", \"sha256\"") << "directory";
        QTest::newRow("file and link") << changed(manifest, "Versions/Current\", \"target\"", "Versions/5/Q\", \"target\"") << "";
        QTest::newRow("link outside") << changed(manifest, "\"target\": \"5\"", "\"target\": \"../../../../../../5\"") << "outside";
        QTest::newRow("link to nothing") << changed(manifest, "\"target\": \"5\"", "\"target\": \"6\"") << "isn't a listed file";
        QTest::newRow("link loop") << changed(manifest, "\"target\": \"5\"", "\"target\": \"Current\"") << "loops";
        QTest::newRow("windows links") << changed(changed(manifest, "\"os\": \"macos\"", "\"os\": \"windows\""), "macos-x86_64-qt5.15", "windows-x86_64-qt5.15") << "Windows";
        QTest::newRow("supersedes itself") << changed(manifest, "[\"macos-x86_64-qt5.6\"]", "[\"macos-x86_64-qt5.15\"]") << "itself";
    }
    void test_unreadableVariant()
    {
        QFETCH(QByteArray, data);
        QFETCH(QString, message);
        BuildManifest build;
        QString error;
        QVERIFY2(parseBuild(data, 5, build, error), qPrintable(error));
        QCOMPARE(build.variants.size(), 1);
        QCOMPARE(build.unreadable.size(), 1);
        auto why = build.unreadable.first();
        QVERIFY2(why.contains(message), qPrintable(why));
    }

    // a Qt 6 variant described in a way this Launcher doesn't know: the Qt 5 one is still used,
    // and the new one is no successor here
    void test_laterVariantFormat()
    {
        QString error;
        BuildManifest build;
        auto data = changed(manifest, "\"macos-x86_64-qt5.15\": {", "\"linux-x86_64-qt6\": {\"name\": \"Linux, Qt 6\", \"supersedes\": [\"linux-x86_64-qt5\"], "
                            "\"requires\": {\"os\": \"linux\", \"arch\": \"x86_64\", \"glibc\": \"2.35\", \"libs\": [{\"soname\": \"libQt6Core.so.6\", \"version\": \"Qt_6.8\"}]}, "
                            "\"files\": [{\"path\": \"bin/MultiMC\", \"sha256\": \"9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08\", \"size\": 4, \"executable\": true}], \"links\": []},\n"
                            "    \"macos-x86_64-qt5.15\": {");
        QVERIFY2(parseBuild(data, 5, build, error), qPrintable(error));
        QCOMPARE(QStringList(build.variants.keys()), QStringList() << "linux-x86_64-qt5" << "macos-x86_64-qt5.15");
        QCOMPARE(QStringList(build.unreadable.keys()), QStringList() << "linux-x86_64-qt6");
        QVERIFY2(build.unreadable.first().contains("requires"), qPrintable(build.unreadable.first()));
    }

    // deprecated (added 2026-11-13): the last day builds of a variant are made; a date that
    // can't be read is ignored, the variant isn't
    void test_deprecated()
    {
        QString error;
        BuildManifest build;
        QVERIFY2(parseBuild(manifest, 5, build, error), qPrintable(error));
        QVERIFY(!build.variants.value("macos-x86_64-qt5.15").deprecated.isValid());

        auto data = changed(manifest, "\"macos-x86_64-qt5.15\": {", "\"macos-x86_64-qt5.15\": {\"deprecated\": \"2027-03-01\", ");
        QVERIFY2(parseBuild(data, 5, build, error), qPrintable(error));
        QCOMPARE(build.variants.value("macos-x86_64-qt5.15").deprecated, QDate(2027, 3, 1));

        for(auto bad: {"\"2027-02-30\"", "\"March 2027\"", "20270301"})
        {
            data = changed(manifest, "\"macos-x86_64-qt5.15\": {", QString("\"macos-x86_64-qt5.15\": {\"deprecated\": %1, ").arg(bad));
            QVERIFY2(parseBuild(data, 5, build, error), qPrintable(error));
            QVERIFY(build.variants.contains("macos-x86_64-qt5.15"));
            QVERIFY(!build.variants.value("macos-x86_64-qt5.15").deprecated.isValid());
        }
    }

    void test_paths()
    {
        QVERIFY(isValidPath("a"));
        QVERIFY(isValidPath("MultiMC.app/Contents/Frameworks/QtCore.framework/Versions/5/QtCore"));
        QVERIFY(isValidPath("../5", true));
        QVERIFY(!isValidPath("../5"));
        QVERIFY(!isValidPath(""));
        QVERIFY(!isValidPath("a//b"));
        QVERIFY(!isValidPath("a/\tb"));
        QVERIFY(!isValidPath(QString(513, 'a')));
        QVERIFY(isValidPath(QString(512, 'a')));
        QCOMPARE(contentPath("9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08"),
                 QString("files/9f/9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08"));
    }
};

QTEST_GUILESS_MAIN(UpdaterV2FormatsTest)

#include "Formats_test.moc"
