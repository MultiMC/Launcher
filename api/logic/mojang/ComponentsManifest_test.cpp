#include <QTest>
#include <QDebug>
#include "TestUtil.h"

#include "mojang/ComponentsManifest.h"

using namespace mojang_files;

class ComponentsManifestTest : public QObject
{
    Q_OBJECT

private slots:
    void test_parse();
    void test_parse_file();
};

namespace {
QByteArray basic_manifest = R"END(
{
    "gamecore": {
        "java-runtime-alpha": [],
        "jre-legacy": [],
        "minecraft-java-exe": []
    },
    "linux": {
        "java-runtime-alpha": [{
            "availability": {
                "group": 5851,
                "progress": 100
            },
            "manifest": {
                "sha1": "e968e71afd3360e5032deac19e1c14d7aa32f5bb",
                "size": 81882,
                "url": "https://launchermeta.mojang.com/v1/packages/e968e71afd3360e5032deac19e1c14d7aa32f5bb/manifest.json"
            },
            "version": {
                "name": "16.0.1.9.1",
                "released": "2021-05-10T16:43:02+00:00"
            }
        }],
        "jre-legacy": [{
            "availability": {
                "group": 6513,
                "progress": 100
            },
            "manifest": {
                "sha1": "a1c15cc788f8893fba7e988eb27404772f699a84",
                "size": 125581,
                "url": "https://launchermeta.mojang.com/v1/packages/a1c15cc788f8893fba7e988eb27404772f699a84/manifest.json"
            },
            "version": {
                "name": "8u202",
                "released": "2020-11-17T19:26:25+00:00"
            }
        }],
        "minecraft-java-exe": []
    }
}
)END";
}

void ComponentsManifestTest::test_parse()
{
    auto manifest = AllPlatformsManifest::fromManifestContents(basic_manifest);
    QVERIFY(manifest.valid == true);
    QVERIFY(manifest.platforms.count("gamecore") == 0);
    QVERIFY(manifest.platforms.count("linux") == 1);
    /*
    QVERIFY(manifest.files.size() == 1);
    QVERIFY(manifest.files.count(Path("a/b.txt")));
    auto &file = manifest.files[Path("a/b.txt")];
    QVERIFY(file.executable == true);
    QVERIFY(file.hash == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    QVERIFY(file.size == 0);
    QVERIFY(manifest.folders.size() == 4);
    QVERIFY(manifest.folders.count(Path(".")));
    QVERIFY(manifest.folders.count(Path("a")));
    QVERIFY(manifest.folders.count(Path("a/b")));
    QVERIFY(manifest.folders.count(Path("a/b/c")));
    QVERIFY(manifest.symlinks.size() == 1);
    auto symlinkPath = Path("a/b/c.txt");
    QVERIFY(manifest.symlinks.count(symlinkPath));
    auto &symlink = manifest.symlinks[symlinkPath];
    QVERIFY(symlink == Path("../b.txt"));
    QVERIFY(manifest.sources.size() == 1);
    */
}

void ComponentsManifestTest::test_parse_file() {
    auto path = QFINDTESTDATA("testdata/all.json");
    auto manifest = AllPlatformsManifest::fromManifestFile(path);
    QVERIFY(manifest.valid == true);
    QVERIFY(manifest.platforms.count("gamecore") == 0);
    QVERIFY(manifest.platforms.count("linux") == 1);
    /*
    QVERIFY(manifest.sources.count("c725183c757011e7ba96c83c1e86ee7e8b516a2b") == 1);
    */
}

QTEST_GUILESS_MAIN(ComponentsManifestTest)

#include "ComponentsManifest_test.moc"
