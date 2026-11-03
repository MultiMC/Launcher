// SPDX-License-Identifier: Apache-2.0
#include "PackUpdate.h"
#include <QtTest>
#include <stdexcept>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <quazip.h>
#include <quazipfile.h>
#include <quazipnewinfo.h>
using namespace Modrinth::Update;

namespace {
void put(const QString &path, const QByteArray &data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size()) throw Error{"fixture write failed"};
}
QByteArray get(const QString &path) { QFile f(path); f.open(QIODevice::ReadOnly); return f.readAll(); }
File entry(const QString &path, const QByteArray &data, const QString &id = QString())
{
    File f;
    f.path = path; f.size = data.size(); f.identity = id;
    f.hash = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha512).toHex());
    f.url = QUrl("https://example.com/" + path);
    return f;
}
Pack pack(const QList<File> &files)
{
    Pack p;
    p.manifest = {{"dependencies", QJsonObject{{"minecraft", "1.21.1"}, {"fabric-loader", "0.16.0"}}}};
    for (auto f : files) p.files.insert(f.path, f);
    return p;
}
void archive(const QString &path, const QMap<QString, QByteArray> &entries)
{
    QuaZip zip(path);
    if (!zip.open(QuaZip::mdCreate)) throw Error{"fixture zip open failed"};
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        QuaZipFile file(&zip);
        if (!file.open(QIODevice::WriteOnly, QuaZipNewInfo(it.key()))) throw Error{"fixture zip failed"};
        file.write(it.value()); file.close();
    }
    zip.close();
}
QString tempRoot(QTemporaryDir &tmp) { return QDir(tmp.path()).canonicalPath(); }
}
class PackUpdateTest : public QObject
{
    Q_OBJECT
private slots:
    void paths()
    {
        QTemporaryDir temp;
        auto root = tempRoot(temp);
        for (auto path : {"../escape", "/absolute", "a/../../escape", "a\\b", "a:b", "a//b", "a/./b", "CON.jar", "a. /b"}) {
            QVERIFY_EXCEPTION_THROWN(safePath(root, path), Error);
        }
        QCOMPARE(safePath(root, "mods/a.jar"), root + "/mods/a.jar");
        put(root + "/target", "x");
        if (QFile::link(root + "/target", root + "/link")) QVERIFY_EXCEPTION_THROWN(safePath(root, "link"), Error);
    }
    void preserveSettingsAndExtras()
    {
        QTemporaryDir temp;
        auto root = tempRoot(temp);
        auto oldPack = pack({entry("mods/old.jar", "old", "mod-a"), entry("config/settings.json", "default")});
        put(root + "/mods/old.jar", "old"); put(root + "/config/settings.json", "default");
        auto old = baseline(oldPack, root, "p", "v1");
        put(root + "/config/settings.json", "my-settings"); put(root + "/options.txt", "keybinds");
        put(root + "/mods/extra.jar", "extra"); put(root + "/saves/world/data", "world");
        auto next = pack({entry("mods/new.jar", "new", "mod-a"), entry("config/settings.json", "new-default"), entry("config/new.json", "new")});
        auto p = plan(old, next, root, "p", "v2");
        QCOMPARE(p.removes, QStringList{"mods/old.jar"});
        QVERIFY(p.writes.contains("mods/new.jar")); QVERIFY(p.writes.contains("config/new.json"));
        QVERIFY(!p.writes.contains("config/settings.json")); QVERIFY(p.skipped.contains("config/settings.json"));
        QVERIFY(!p.removes.contains("mods/extra.jar"));
        verifyUnchanged(root, p);
        put(root + "/options.txt", "new-keys");
        QVERIFY_EXCEPTION_THROWN(verifyUnchanged(root, p), Error);
    }
    void disabledAndUnchanged()
    {
        QTemporaryDir temp; auto root = tempRoot(temp);
        auto original = pack({entry("mods/a.jar", "a", "id-a"), entry("mods/b.jar", "b", "id-b")});
        put(root + "/mods/a.jar.disabled", "a"); put(root + "/mods/b.jar", "b");
        auto old = baseline(original, root, "p", "v1");
        auto next = pack({entry("mods/a2.jar", "a2", "id-a"), entry("mods/b.jar", "b", "id-b")});
        auto p = plan(old, next, root, "p", "v2");
        QVERIFY(p.writes.contains("mods/a2.jar.disabled")); QVERIFY(!p.writes.contains("mods/b.jar"));
        QCOMPARE(p.downloadBytes, qint64(2));
        QCOMPARE(p.removes, QStringList{"mods/a.jar.disabled"});
    }
    void modifiedMissingAndCollidingMods()
    {
        QTemporaryDir temp; auto root = tempRoot(temp);
        auto original = pack({entry("mods/a.jar", "a")});
        put(root + "/mods/a.jar", "a");
        auto old = baseline(original, root, "p", "v1");
        put(root + "/mods/a.jar", "manual");
        QVERIFY_EXCEPTION_THROWN(plan(old, original, root, "p", "v2"), Error);
        QVERIFY_EXCEPTION_THROWN(baseline(original, root, "p", "v1"), Error);
        QFile::remove(root + "/mods/a.jar");
        QVERIFY_EXCEPTION_THROWN(plan(old, original, root, "p", "v2"), Error);
        put(root + "/mods/a.jar", "a"); put(root + "/mods/new.jar", "user");
        auto next = pack({entry("mods/new.jar", "pack")});
        QVERIFY_EXCEPTION_THROWN(plan(old, next, root, "p", "v2"), Error);
    }
    void loaderComponentUpdate()
    {
        QJsonObject minecraft{{"uid", "net.minecraft"}, {"version", "1.21.1"}};
        QJsonObject loader{{"uid", "net.fabricmc.fabric-loader"}, {"version", "0.16.0"}, {"cachedVersion", "0.16.0"},
                           {"cachedRequires", QJsonArray{QJsonObject{{"uid", "old-dependency"}}}}};
        QJsonObject original{{"formatVersion", 1}, {"components", QJsonArray{minecraft, loader}}};
        auto updated = updateLoaderComponent(original, "fabric-loader", "0.16.1");
        auto list = updated["components"].toArray();
        QCOMPARE(list[0].toObject(), minecraft);
        QCOMPARE(list[1].toObject()["version"].toString(), QString("0.16.1"));
        QVERIFY(!list[1].toObject().contains("cachedRequires"));
        QVERIFY(!list[1].toObject().contains("cachedVersion"));
        QVERIFY_EXCEPTION_THROWN(updateLoaderComponent(original, "forge", "1"), Error);
    }
    void versionSelection()
    {
        auto current = QJsonObject{{"date_published", "2025-01-01T00:00:00Z"}};
        auto v = QJsonObject{{"id", "new"}, {"date_published", "2025-02-01T00:00:00Z"}, {"version_type", "release"}, {"game_versions", QJsonArray{"1.21.1"}}, {"loaders", QJsonArray{"fabric"}}};
        auto beta = v; beta["version_type"] = "beta"; beta["date_published"] = "2025-03-01T00:00:00Z";
        auto archived = v; archived["status"] = "archived"; archived["date_published"] = "2025-04-01T00:00:00Z";
        QVERIFY(latestRelease({archived}, "1.21.1", "fabric-loader", current).isEmpty());
        auto wrong = v; wrong["game_versions"] = QJsonArray{"1.22"};
        QCOMPARE(latestRelease({beta, v, wrong}, "1.21.1", "fabric-loader", current)["id"].toString(), QString("new"));
        QVERIFY(latestRelease({beta, wrong}, "1.21.1", "fabric-loader", current).isEmpty());
        QVERIFY(latestRelease({v}, "1.21.1", "forge", current).isEmpty());
        QVERIFY(latestRelease({v}, "1.21.1", "fabric-loader", v).isEmpty());
        validateCompatibility(pack({}), "1.21.1", "fabric-loader");
        QVERIFY_EXCEPTION_THROWN(validateCompatibility(pack({}), "1.22", "fabric-loader"), Error);
    }
    void archiveValidation()
    {
        QTemporaryDir temp; auto root = tempRoot(temp);
        auto index = QJsonObject{{"formatVersion", 1}, {"game", "minecraft"}, {"files", QJsonArray{}}};
        auto json = QJsonDocument(index).toJson();
        archive(root + "/good.mrpack", {{"modrinth.index.json", json}, {"overrides/config/a", "base"}, {"client-overrides/config/a", "client"}});
        auto p = readPack(root + "/good.mrpack", root + "/out");
        QCOMPARE(get(root + "/out/config/a"), QByteArray("client")); QVERIFY(p.files.contains("config/a"));
        archive(root + "/bad.mrpack", {{"modrinth.index.json", json}, {"overrides/../../escape", "bad"}});
        QVERIFY_EXCEPTION_THROWN(readPack(root + "/bad.mrpack", root + "/out2"), Error);
        index["files"] = QJsonArray{QJsonObject{{"path", "mods/a.jar"}, {"hashes", QJsonObject{{"sha512", "invalid"}}}}};
        archive(root + "/bad-hash.mrpack", {{"modrinth.index.json", QJsonDocument(index).toJson()}});
        QVERIFY_EXCEPTION_THROWN(readPack(root + "/bad-hash.mrpack", root + "/out3"), Error);
    }
    void duplicateIds()
    {
        QTemporaryDir temp; auto root = tempRoot(temp);
        QDir().mkpath(root + "/game/mods"); QDir().mkpath(root + "/stage/mods");
        archive(root + "/game/mods/a.jar", {{"fabric.mod.json", "{\"id\":\"same\"}"}});
        archive(root + "/stage/mods/b.jar", {{"fabric.mod.json", "{\"id\":\"same\"}"}});
        Plan p; p.writes.insert("mods/b.jar", entry("mods/b.jar", ""));
        QVERIFY_EXCEPTION_THROWN(validateMods(root + "/game", root + "/stage", p), Error);
        p.removes.append("mods/a.jar");
        validateMods(root + "/game", root + "/stage", p);
    }
    void transactionAndRestore()
    {
        QTemporaryDir temp; auto root = tempRoot(temp) + "/instance"; auto stage = tempRoot(temp) + "/stage";
        put(root + "/.minecraft/mods/a.jar", "a"); put(root + "/.minecraft/options.txt", "keys"); put(root + "/modrinth-update.json", "old");
        put(stage + "/.minecraft/mods/b.jar", "b"); put(stage + "/modrinth-update.json", "new");
        commit(root, stage, {".minecraft/mods/b.jar", "modrinth-update.json"}, {".minecraft/mods/a.jar"});
        QVERIFY(!pending(root)); QVERIFY(hasBackup(root));
        QCOMPARE(get(root + "/.minecraft/mods/b.jar"), QByteArray("b"));
        QVERIFY(!QFileInfo::exists(root + "/.minecraft/mods/a.jar"));
        QCOMPARE(get(root + "/.minecraft/options.txt"), QByteArray("keys"));
        put(root + "/.minecraft/saves/new-world", "keep");
        restore(root);
        QCOMPARE(get(root + "/.minecraft/mods/a.jar"), QByteArray("a"));
        QVERIFY(!QFileInfo::exists(root + "/.minecraft/mods/b.jar"));
        QCOMPARE(get(root + "/modrinth-update.json"), QByteArray("old"));
        QCOMPARE(get(root + "/.minecraft/saves/new-world"), QByteArray("keep"));
    }
    void failureRollsBack()
    {
        QTemporaryDir temp; auto root = tempRoot(temp) + "/instance"; auto stage = tempRoot(temp) + "/stage";
        put(root + "/a", "original"); put(stage + "/a", "updated");
        QVERIFY_EXCEPTION_THROWN(commit(root, stage, {"a", "missing"}, {}), Error);
        QCOMPARE(get(root + "/a"), QByteArray("original")); QVERIFY(!pending(root));
    }
    void failedSecondUpdatePreservesRestorePoint()
    {
        QTemporaryDir temp; auto root = tempRoot(temp) + "/instance"; auto stage = tempRoot(temp) + "/stage";
        put(root + "/a", "first"); put(stage + "/a", "second");
        commit(root, stage, {"a"}, {});
        const auto previous = readJson(root + "/.pack-update/previous.json");
        put(stage + "/a", "third"); put(stage + "/b", "new");
        QVERIFY_EXCEPTION_THROWN(commit(root, stage, {"a", "b"}, {}, [](const QString &status) {
            if (status == "Installing b") throw Error{"injected commit failure"};
        }), Error);
        QCOMPARE(get(root + "/a"), QByteArray("second"));
        QCOMPARE(readJson(root + "/.pack-update/previous.json"), previous);
        QVERIFY(!pending(root));
        restore(root);
        QCOMPARE(get(root + "/a"), QByteArray("first"));
    }
    void interruptedSecondUpdateIsRecoverable()
    {
        QTemporaryDir temp; auto root = tempRoot(temp) + "/instance"; auto stage = tempRoot(temp) + "/stage";
        put(root + "/a", "first"); put(stage + "/a", "second");
        commit(root, stage, {"a"}, {});
        put(stage + "/a", "third"); put(stage + "/b", "new");
        // A non-Error exception simulates termination without automatic rollback.
        QVERIFY_EXCEPTION_THROWN(commit(root, stage, {"a", "b"}, {}, [](const QString &status) {
            if (status == "Installing b") throw std::runtime_error("interrupted");
        }), std::runtime_error);
        QVERIFY(pending(root));
        QCOMPARE(get(root + "/a"), QByteArray("third"));
        restore(root);
        QCOMPARE(get(root + "/a"), QByteArray("second"));
        QVERIFY(hasBackup(root));
        restore(root);
        QCOMPARE(get(root + "/a"), QByteArray("first"));
    }
    void malformedJournalCannotDeleteFiles()
    {
        QTemporaryDir temp; auto root = tempRoot(temp);
        put(root + "/a", "user data");
        for (auto paths : {QJsonObject{{"a", QJsonValue::Null}}, QJsonObject{{".pack-update/previous.json", ""}}}) {
            writeJson(root + "/.pack-update/journal.json", {{"schema", 1}, {"paths", paths}});
            QVERIFY_EXCEPTION_THROWN(restore(root), Error);
            QCOMPARE(get(root + "/a"), QByteArray("user data"));
            QVERIFY(pending(root));
        }
    }
    void caseCollision()
    {
        QTemporaryDir temp; auto root = tempRoot(temp);
        put(root + "/mods/User.jar", "user");
        auto old = baseline(pack({}), root, "p", "v1");
        QVERIFY_EXCEPTION_THROWN(plan(old, pack({entry("mods/user.jar", "new")}), root, "p", "v2"), Error);
    }
    void importSafetyDoesNotRequireUpdaterManifest()
    {
        QTemporaryDir temp; auto root = tempRoot(temp);
        archive(root + "/pack.zip", {{"nested/modrinth.index.json", "{}"}});
        validateArchive(root + "/pack.zip", root + "/out");
        archive(root + "/bad.zip", {{"../escape", "bad"}});
        QVERIFY_EXCEPTION_THROWN(validateArchive(root + "/bad.zip", root + "/out"), Error);
    }
    void changedSupportFilesAndResourcePacks()
    {
        QTemporaryDir temp; auto root = tempRoot(temp);
        auto original = pack({entry("resourcepacks/a.zip", "old"), entry("packcore/code.js", "old"), entry("config/a.json", "old")});
        for (const auto &f : original.files) put(root + '/' + f.path, "old");
        auto old = baseline(original, root, "p", "v1");
        auto target = pack({entry("resourcepacks/b.zip", "new"), entry("packcore/code.js", "new"), entry("config/a.json", "new")});
        auto p = plan(old, target, root, "p", "v2");
        QVERIFY(p.writes.contains("packcore/code.js")); QVERIFY(p.writes.contains("resourcepacks/b.zip"));
        QVERIFY(p.removes.contains("resourcepacks/a.zip")); QVERIFY(!p.writes.contains("config/a.json"));
        put(root + "/packcore/code.js", "customized");
        p = plan(old, target, root, "p", "v2");
        QVERIFY(!p.writes.contains("packcore/code.js")); QVERIFY(p.skipped.contains("packcore/code.js"));
    }
    void insufficientSpace()
    {
        QTemporaryDir temp; auto root = tempRoot(temp) + "/instance"; auto stage = tempRoot(temp) + "/stage";
        put(root + "/a", "original"); put(stage + "/a", "updated");
        QVERIFY_EXCEPTION_THROWN(commit(root, stage, {"a"}, {}, Progress(), []() { return qint64(0); }), Error);
        QCOMPARE(get(root + "/a"), QByteArray("original"));
        QVERIFY(!pending(root)); QVERIFY(!hasBackup(root));
    }
    void backupChangeAndCorruption()
    {
        QTemporaryDir temp; auto root = tempRoot(temp) + "/instance"; auto stage = tempRoot(temp) + "/stage";
        put(root + "/a", "original"); put(stage + "/a", "new");
        QVERIFY_EXCEPTION_THROWN(commit(root, stage, {"a"}, {}, [&](const QString &status) {
            if (status.startsWith("Backing up")) throw Error{"injected backup failure"};
        }), Error);
        QCOMPARE(get(root + "/a"), QByteArray("original")); QVERIFY(!pending(root));
        commit(root, stage, {"a"}, {});
        put(root + "/.pack-update/" + readJson(root + "/.pack-update/previous.json")["backup"].toString() + "/a", "damaged");
        QVERIFY_EXCEPTION_THROWN(restore(root), Error);
        QCOMPARE(get(root + "/a"), QByteArray("new"));
    }
    void forgeDependenciesAreNotModIds()
    {
        QTemporaryDir temp; auto root = tempRoot(temp);
        archive(root + "/forge.jar", {{"META-INF/mods.toml", "[[mods]]\nmodId='one'\n[[mods]]\nmodId='two'\n[[dependencies.one]]\nmodId='minecraft'\n"}});
        QCOMPARE(modIds(root + "/forge.jar"), (QStringList{"one", "two"}));
    }
    void skyblockAcceptance()
    {
        auto fixtures = QString::fromLocal8Bit(qgetenv("MULTIMC_SKYBLOCK_FIXTURES"));
        if (fixtures.isEmpty()) QSKIP("Set MULTIMC_SKYBLOCK_FIXTURES for real-pack acceptance (see doc/modpack-updater.md)");
        QTemporaryDir temp; auto root = tempRoot(temp) + "/instance"; auto game = root + "/.minecraft";
        auto oldPack = readPack(fixtures + "/old.mrpack", tempRoot(temp) + "/old-overrides");
        auto newPack = readPack(fixtures + "/new.mrpack", tempRoot(temp) + "/new-overrides");
        auto oldRelease = readJson(fixtures + "/old.json"), newRelease = readJson(fixtures + "/new.json");
        auto deps = oldPack.manifest.value("dependencies").toObject();
        validateCompatibility(newPack, deps.value("minecraft").toString(), loaderKey(deps));
        QCOMPARE(latestRelease({newRelease}, deps.value("minecraft").toString(), loaderKey(deps), oldRelease)["id"], newRelease["id"]);
        QString disabled;
        for (const auto &f : oldPack.files) {
            auto from = f.overrideFile ? tempRoot(temp) + "/old-overrides/" + f.path : fixtures + "/cache/" + f.algorithm + '-' + f.hash;
            QCOMPARE(digest(from, f.algorithm), f.hash);
            auto actual = f.path + (f.optional ? ".disabled" : "");
            if (disabled.isEmpty() && f.path.startsWith("mods/") && !f.identity.isEmpty()) { disabled = f.path; actual = f.path + ".disabled"; }
            QDir().mkpath(QFileInfo(game + '/' + actual).absolutePath());
            QVERIFY(QFile::copy(from, game + '/' + actual));
        }
        auto old = baseline(oldPack, game, oldRelease["project_id"].toString(), oldRelease["id"].toString());
        writeJson(root + "/modrinth-update.json", old);
        put(game + "/options.txt", "user keybinds");
        put(game + "/config/user-preferences.json", "user config");
        QString changedConfig;
        for (const auto &f : oldPack.files) if (f.path.startsWith("config/")) { changedConfig = f.path; put(game + '/' + f.path, "custom preferences"); break; }
        archive(game + "/mods/user-extra.jar", {{"fabric.mod.json", "{\"id\":\"updater_acceptance_extra\"}"}});
        auto p = plan(old, newPack, game, newRelease["project_id"].toString(), newRelease["id"].toString());
        QVERIFY(!p.writes.isEmpty());
        auto payload = tempRoot(temp) + "/payload", stagedGame = payload + "/.minecraft";
        QStringList writes, removes;
        for (const auto &f : p.writes) {
            auto originalPath = f.path.endsWith(".disabled") ? f.path.left(f.path.size() - 9) : f.path;
            auto from = f.overrideFile ? tempRoot(temp) + "/new-overrides/" + originalPath : fixtures + "/cache/" + f.algorithm + '-' + f.hash;
            QCOMPARE(digest(from, f.algorithm), f.hash);
            QDir().mkpath(QFileInfo(stagedGame + '/' + f.path).absolutePath());
            QVERIFY(QFile::copy(from, stagedGame + '/' + f.path));
            writes.append(".minecraft/" + f.path);
        }
        for (auto path : p.removes) removes.append(".minecraft/" + path);
        validateMods(game, stagedGame, p);
        verifyUnchanged(game, p);
        writeJson(payload + "/modrinth-update.json", p.metadata); writes.append("modrinth-update.json");
        commit(root, payload, writes, removes);
        QCOMPARE(get(game + "/options.txt"), QByteArray("user keybinds"));
        QCOMPARE(get(game + "/config/user-preferences.json"), QByteArray("user config"));
        if (!changedConfig.isEmpty()) QCOMPARE(get(game + '/' + changedConfig), QByteArray("custom preferences"));
        QVERIFY(QFileInfo::exists(game + "/mods/user-extra.jar"));
        auto again = plan(p.metadata, newPack, game, newRelease["project_id"].toString(), newRelease["id"].toString());
        QVERIFY(again.writes.isEmpty()); QVERIFY(again.removes.isEmpty());
        restore(root);
        QCOMPARE(readJson(root + "/modrinth-update.json"), old);
        QVERIFY(QFileInfo::exists(game + '/' + disabled + ".disabled"));
        QCOMPARE(get(game + "/options.txt"), QByteArray("user keybinds"));
        qInfo() << "Updated and restored SkyBlock Enhanced" << oldRelease["version_number"].toString() << "to" << newRelease["version_number"].toString() << "with" << p.writes.size() << "writes and" << p.removes.size() << "removals";
    }
    void interruptedRecovery()
    {
        QTemporaryDir temp; auto root = tempRoot(temp);
        put(root + "/a", "partial"); put(root + "/new", "partial");
        put(root + "/.pack-update/backup/a", "original");
        writeJson(root + "/.pack-update/journal.json", {{"schema", 1}, {"paths", QJsonObject{{"a", digest(root + "/.pack-update/backup/a")}, {"new", ""}}}});
        QVERIFY(pending(root)); restore(root); QVERIFY(!pending(root));
        QCOMPARE(get(root + "/a"), QByteArray("original")); QVERIFY(!QFileInfo::exists(root + "/new"));
    }
};
QTEST_GUILESS_MAIN(PackUpdateTest)
#include "PackUpdate_test.moc"
