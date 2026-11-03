// SPDX-License-Identifier: Apache-2.0
#include "PackUpdate.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStorageInfo>
#include <QUuid>
#include <quazip.h>
#include <quazipfile.h>
#include <toml.h>

namespace Modrinth {
namespace Update {
namespace {
void require(bool ok, const QString &message) { if (!ok) throw Error{message}; }
bool exists(const QString &path) { return QFileInfo::exists(path) || QFileInfo(path).isSymLink(); }
void copyFile(const QString &from, const QString &to)
{
    require(QDir().mkpath(QFileInfo(to).absolutePath()), "Cannot create directory: " + to);
    QFile input(from);
    QSaveFile output(to);
    require(input.open(QIODevice::ReadOnly) && output.open(QIODevice::WriteOnly), "Cannot copy: " + from);
    while (!input.atEnd()) {
        auto data = input.read(1024 * 1024);
        require(!data.isEmpty() && output.write(data) == data.size(), "Copy failed: " + from);
    }
    require(output.commit(), "Cannot commit file: " + to);
    QFile::setPermissions(to, input.permissions());
}
QStringList tree(const QString &root, bool excludeState = false)
{
    QStringList result;
    std::function<void(const QString &)> visit = [&](const QString &relativeDir) {
        const auto directory = relativeDir.isEmpty() ? root : safePath(root, relativeDir);
        const auto entries = QDir(directory).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const auto &entry : entries) {
            const auto relative = relativeDir.isEmpty() ? entry.fileName() : relativeDir + '/' + entry.fileName();
            if (excludeState && relative == ".pack-update") continue;
            safePath(root, relative);
            require(entry.isFile() || entry.isDir(), "Unsupported file: " + relative);
            if (entry.isDir()) visit(relative);
            else result.append(relative);
        }
    };
    visit(QString());
    result.sort();
    return result;
}
QString identity(const File &file)
{
    auto parts = file.url.path().split('/');
    int data = parts.indexOf("data");
    if (file.url.host() == "cdn.modrinth.com" && data >= 0 && parts.size() > data + 1)
        return "modrinth:" + parts[data + 1];
    return file.identity;
}
QJsonObject record(const File &file, const QString &actual)
{
    return {{"path", actual}, {"hash", file.hash}, {"algorithm", file.algorithm},
            {"identity", identity(file)}, {"optional", file.optional}, {"override", file.overrideFile}};
}
QString actualPath(const QString &root, const File &file)
{
    auto normal = safePath(root, file.path);
    if (!file.path.startsWith("mods/") && !file.optional) return file.path;
    auto disabled = safePath(root, file.path + ".disabled");
    require(!(exists(normal) && exists(disabled)), "Both enabled and disabled copies exist: " + file.path);
    return exists(disabled) ? file.path + ".disabled" : file.path;
}
bool isMod(const QString &path) { return path.startsWith("mods/"); }
bool protectedData(const QString &path)
{
    auto p = path.toLower();
    return p.startsWith("config/") || p.startsWith("defaultconfigs/") || p.startsWith("saves/") ||
        p.startsWith("screenshots/") || p.startsWith("logs/") || p.startsWith("crash-reports/") ||
        p.startsWith("options") || p == "servers.dat" || p == "servers.dat_old" || p == "usercache.json";
}
QString requiredString(const QJsonObject &o, const QString &key)
{
    auto value = o.value(key).toString();
    require(!value.isEmpty(), "Missing or invalid field: " + key);
    return value;
}
QJsonObject state(const Pack &pack, const QString &project, const QString &version, const QJsonObject &files)
{
    return {{"schema", 1}, {"project", project}, {"version", version}, {"manifest", pack.manifest}, {"files", files}};
}
void validateState(const QJsonObject &state)
{
    require(state.value("schema").toInt() == 1 && state.value("files").isObject() && state.value("manifest").isObject(),
            "Unsupported or damaged pack update metadata. Link the original release again.");
}
}

QString safePath(const QString &root, const QString &relative)
{
    require(!relative.isEmpty() && !QDir::isAbsolutePath(relative) && !relative.contains('\\') && !relative.contains(':') && !relative.contains(QChar(0)),
            "Unsafe path: " + relative);
    QString path = QDir(root).absolutePath();
    require(!QFileInfo(path).isSymLink(), "Symlink root is not supported: " + path);
    // Check ancestors too: a non-symlink root can itself be below a symlink.
    QString ancestor = path;
    while (ancestor != QFileInfo(ancestor).dir().absolutePath()) {
        require(!QFileInfo(ancestor).isSymLink(), "Symlink ancestor: " + ancestor);
        ancestor = QFileInfo(ancestor).dir().absolutePath();
    }
    for (const auto &part : relative.split('/')) {
        require(!part.isEmpty() && part != "." && part != ".." && !part.endsWith('.') && !part.endsWith(' '), "Unsafe path: " + relative);
        require(!QRegularExpression("^(con|prn|aux|nul|com[1-9]|lpt[1-9])(\\..*)?$", QRegularExpression::CaseInsensitiveOption).match(part).hasMatch(), "Reserved path: " + relative);
        path += '/' + part;
        require(!QFileInfo(path).isSymLink(), "Symlinks are not supported: " + path);
    }
    return path;
}
QString digest(const QString &path, const QString &algorithm)
{
    require(algorithm == "sha512" || algorithm == "sha256" || algorithm == "sha1", "Unsupported hash algorithm");
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "Cannot read: " + path);
    QCryptographicHash hash(algorithm == "sha512" ? QCryptographicHash::Sha512 : algorithm == "sha256" ? QCryptographicHash::Sha256 : QCryptographicHash::Sha1);
    require(hash.addData(&file), "Cannot hash: " + path);
    return QString::fromLatin1(hash.result().toHex());
}
QJsonObject readJson(const QString &path)
{
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "Cannot read: " + path);
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    require(error.error == QJsonParseError::NoError && doc.isObject(), "Invalid JSON: " + path);
    return doc.object();
}
void writeJson(const QString &path, const QJsonObject &object)
{
    require(QDir().mkpath(QFileInfo(path).absolutePath()), "Cannot create directory for: " + path);
    QSaveFile file(path);
    auto data = QJsonDocument(object).toJson();
    require(file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit(), "Cannot save: " + path);
}

void validateArchive(const QString &archive, const QString &destination)
{
    QuaZip zip(archive);
    require(zip.open(QuaZip::mdUnzip), "Cannot open modpack archive");
    QSet<QString> seen;
    for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile()) {
        QuaZipFileInfo64 info;
        require(zip.getCurrentFileInfo(&info), "Cannot read archive entry");
        auto path = info.name;
        if (path.endsWith('/')) path.chop(1);
        safePath(destination, path);
        require(!seen.contains(path.toCaseFolded()), "Duplicate archive path: " + path);
        seen.insert(path.toCaseFolded());
        require(((info.externalAttr >> 16) & 0170000) != 0120000, "Archive contains a symlink: " + path);
    }
    require(zip.getZipError() == UNZ_OK, "Damaged modpack archive");
}

Pack readPack(const QString &archive, const QString &overrides)
{
    Pack pack;
    QuaZip zip(archive);
    require(zip.open(QuaZip::mdUnzip), "Cannot open modpack archive");
    QSet<QString> seen;
    QMap<QString, QByteArray> defaults, clientDefaults;
    qint64 expanded = 0;
    for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile()) {
        QuaZipFileInfo64 info;
        require(zip.getCurrentFileInfo(&info), "Cannot read archive entry");
        auto path = info.name;
        if (path.endsWith('/')) path.chop(1);
        safePath(overrides, path);
        require(!seen.contains(path.toCaseFolded()), "Duplicate archive path: " + path);
        seen.insert(path.toCaseFolded());
        require(((info.externalAttr >> 16) & 0170000) != 0120000, "Archive contains a symlink: " + path);
        if (info.name.endsWith('/')) continue;
        expanded += info.uncompressedSize;
        require(info.uncompressedSize <= 512ULL * 1024 * 1024 && expanded <= 2LL * 1024 * 1024 * 1024, "Modpack archive is too large");
        QuaZipFile entry(&zip);
        require(entry.open(QIODevice::ReadOnly), "Cannot read archive entry: " + path);
        // Bound the read as well as checking the ZIP directory declaration.
        auto bytes = entry.read(qint64(info.uncompressedSize) + 1);
        entry.close();
        require(entry.getZipError() == UNZ_OK && quint64(bytes.size()) == info.uncompressedSize, "Damaged archive entry: " + path);
        if (path == "modrinth.index.json") {
            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(bytes, &error);
            require(error.error == QJsonParseError::NoError && doc.isObject(), "Invalid modpack manifest");
            pack.manifest = doc.object();
        } else if (path.startsWith("overrides/")) defaults.insert(path.mid(10), bytes);
        else if (path.startsWith("client-overrides/")) clientDefaults.insert(path.mid(17), bytes);
    }
    require(zip.getZipError() == UNZ_OK, "Damaged modpack archive");
    require(pack.manifest.value("formatVersion").toInt() == 1 && pack.manifest.value("game").toString() == "minecraft" && pack.manifest.value("files").isArray(), "Unsupported modpack manifest");
    QSet<QString> paths;
    for (auto value : pack.manifest.value("files").toArray()) {
        auto o = value.toObject();
        File file;
        file.path = requiredString(o, "path");
        safePath(overrides, file.path);
        require(!paths.contains(file.path.toCaseFolded()), "Duplicate manifest path: " + file.path);
        paths.insert(file.path.toCaseFolded());
        auto env = o.value("env").toObject().value("client").toString("required");
        require(env == "unsupported" || env == "optional" || env == "required", "Unknown client environment");
        if (env == "unsupported") continue;
        file.optional = env == "optional";
        auto hashes = o.value("hashes").toObject();
        file.algorithm = hashes.contains("sha512") ? "sha512" : hashes.contains("sha256") ? "sha256" : "sha1";
        file.hash = requiredString(hashes, file.algorithm).toLower();
        int length = file.algorithm == "sha512" ? 128 : file.algorithm == "sha256" ? 64 : 40;
        require(QRegularExpression("^[0-9a-f]{" + QString::number(length) + "}$").match(file.hash).hasMatch(), "Invalid file hash");
        auto downloads = o.value("downloads").toArray();
        require(!downloads.isEmpty(), "Missing download URL: " + file.path);
        file.url = QUrl(downloads.first().toString());
        require(file.url.isValid() && file.url.scheme() == "https" && !file.url.host().isEmpty() && file.url.userInfo().isEmpty(), "Invalid download URL: " + file.path);
        const auto declaredSize = o.value("fileSize").toDouble(-1);
        require(declaredSize >= 0 && declaredSize <= 9007199254740991.0, "Invalid file size");
        file.size = qint64(declaredSize);
        require(double(file.size) == declaredSize, "Invalid file size");
        file.identity = identity(file);
        pack.files.insert(file.path, file);
    }
    for (auto it = clientDefaults.begin(); it != clientDefaults.end(); ++it) defaults.insert(it.key(), it.value());
    for (auto it = defaults.begin(); it != defaults.end(); ++it) {
        auto path = safePath(overrides, it.key());
        require(!paths.contains(it.key().toCaseFolded()), "Override conflicts with manifest: " + it.key());
        paths.insert(it.key().toCaseFolded());
        require(QDir().mkpath(QFileInfo(path).absolutePath()), "Cannot stage override");
        QSaveFile file(path);
        require(file.open(QIODevice::WriteOnly) && file.write(it.value()) == it.value().size() && file.commit(), "Cannot stage override");
        File f;
        f.path = it.key(); f.hash = digest(path); f.size = it.value().size(); f.overrideFile = true;
        if (isMod(f.path) && f.path.endsWith(".jar")) f.identity = modIds(path).join(',');
        pack.files.insert(f.path, f);
    }
    return pack;
}
QString loaderKey(const QJsonObject &deps)
{
    QString loader;
    for (auto it = deps.begin(); it != deps.end(); ++it) {
        if (it.key() == "minecraft") continue;
        require(it.key() == "fabric-loader" || it.key() == "quilt-loader" || it.key() == "forge" || it.key() == "neoforge", "Unsupported pack dependency: " + it.key());
        require(loader.isEmpty(), "Multiple mod loaders are not supported");
        loader = it.key();
    }
    return loader;
}
QString componentUid(const QString &loader)
{
    if (loader == "fabric-loader") return "net.fabricmc.fabric-loader";
    if (loader == "quilt-loader") return "org.quiltmc.quilt-loader";
    if (loader == "forge") return "net.minecraftforge";
    if (loader == "neoforge") return "net.neoforged";
    return QString();
}
QJsonObject updateLoaderComponent(QJsonObject components, const QString &loader, const QString &version)
{
    auto uid = componentUid(loader);
    require(!uid.isEmpty() && !version.isEmpty() && components.value("components").isArray(), "Invalid loader component update");
    auto list = components.value("components").toArray();
    int found = 0;
    for (int i = 0; i < list.size(); ++i) {
        auto c = list[i].toObject();
        if (c.value("uid").toString() != uid) continue;
        c.insert("version", version);
        // Cached dependency constraints belong to the old loader release.
        for (auto key : {"cachedVersion", "cachedRequires", "cachedConflicts", "cachedVolatile"}) c.remove(key);
        list[i] = c;
        ++found;
    }
    require(found == 1, "Missing or ambiguous loader component");
    components.insert("components", list);
    return components;
}
void validateCompatibility(const Pack &pack, const QString &minecraft, const QString &loader)
{
    auto deps = pack.manifest.value("dependencies").toObject();
    require(requiredString(deps, "minecraft") == minecraft && loaderKey(deps) == loader, "This release changes Minecraft version or loader family");
    if (!loader.isEmpty()) requiredString(deps, loader);
}
QJsonObject latestRelease(const QJsonArray &versions, const QString &minecraft, const QString &loader, const QJsonObject &current)
{
    QJsonObject best;
    auto date = QDateTime::fromString(current.value("date_published").toString(), Qt::ISODate);
    require(date.isValid(), "The installed release has no valid publication date");
    QString apiLoader = loader;
    apiLoader.remove("-loader");
    for (auto value : versions) {
        auto v = value.toObject();
        auto published = QDateTime::fromString(v.value("date_published").toString(), Qt::ISODate);
        if (v.value("version_type").toString() != "release" || v.value("status").toString("listed") != "listed" || !v.value("game_versions").toArray().contains(minecraft) ||
            (!apiLoader.isEmpty() && !v.value("loaders").toArray().contains(apiLoader)) || published <= date || !published.isValid()) continue;
        best = v;
        date = published;
    }
    return best;
}
QJsonObject baseline(const Pack &pack, const QString &gameRoot, const QString &project, const QString &version)
{
    QJsonObject files;
    for (const auto &file : pack.files) {
        auto actual = actualPath(gameRoot, file);
        auto path = safePath(gameRoot, actual);
        if (!exists(path) || digest(path, file.algorithm) != file.hash) {
            require(!isMod(file.path), "Cannot link modified or missing pack mod: " + file.path + "\nRestore it from the selected original release, or select the correct release, then retry.");
            continue; // Existing user settings are never claimed as pack-owned.
        }
        files.insert(file.path, record(file, actual));
    }
    return state(pack, project, version, files);
}
Plan plan(const QJsonObject &old, const Pack &target, const QString &root, const QString &project, const QString &version)
{
    validateState(old);
    require(old.value("project").toString() == project, "Pack project does not match");
    Plan p;
    // Record the whole game tree so edits made while a dialog/download is open cannot be overwritten.
    for (auto path : tree(root)) p.observed.insert(path, digest(safePath(root, path)));
    QJsonObject managed = old.value("files").toObject(), next;
    QMap<QString, QString> oldActual;
    QMap<QString, bool> disabled;
    for (auto it = managed.begin(); it != managed.end(); ++it) {
        auto r = it.value().toObject();
        File f; f.path = it.key(); f.optional = r.value("optional").toBool();
        auto actual = actualPath(root, f);
        auto path = safePath(root, actual);
        if (isMod(it.key())) {
            require(exists(path) && digest(path, requiredString(r, "algorithm")) == requiredString(r, "hash"),
                    "Pack mod was modified or deleted: " + actual + "\nRestore its recorded version before updating. Your files have not been changed.");
            auto id = r.value("identity").toString();
            if (!id.isEmpty()) {
                require(!disabled.contains(id), "Ambiguous pack mod identity: " + id);
                disabled.insert(id, actual.endsWith(".disabled"));
            }
        }
        oldActual.insert(it.key(), actual);
    }
    for (const auto &file : target.files) {
        QString actual = file.path;
        if (isMod(file.path) || file.optional) {
            bool off = file.optional;
            if (oldActual.contains(file.path)) off = oldActual[file.path].endsWith(".disabled");
            else if (disabled.contains(identity(file))) off = disabled[identity(file)];
            actual += off ? ".disabled" : "";
        }
        auto path = safePath(root, actual);
        // Refuse case-only aliases consistently on case-sensitive and insensitive disks.
        for (auto it = p.observed.begin(); it != p.observed.end(); ++it)
            require(it.key() == actual || it.key().compare(actual, Qt::CaseInsensitive) != 0,
                    "Case-conflicting instance file: " + it.key() + " and " + actual);
        bool present = exists(path);
        bool owned = managed.contains(file.path) && oldActual[file.path] == actual;
        auto previous = managed.value(file.path).toObject();
        bool modified = present && owned && digest(path, requiredString(previous, "algorithm")) != requiredString(previous, "hash");
        if (present && !isMod(file.path) && (protectedData(file.path) || !owned || modified)) {
            if (digest(path, file.algorithm) != file.hash) p.skipped.append(actual);
            // Keep the old ownership baseline, never adopt modified settings.
            if (managed.contains(file.path)) next.insert(file.path, managed[file.path]);
            continue;
        }
        if (present && !owned) {
            require(digest(path, file.algorithm) == file.hash, "A user file conflicts with this release: " + actual);
            // Identical user content remains unowned, so a later update cannot delete it.
            continue;
        }
        if (!present || digest(path, file.algorithm) != file.hash) {
            auto write = file; write.path = actual;
            p.writes.insert(actual, write);
            (owned ? p.changed : p.added).append(actual);
            if (!file.overrideFile) p.downloadBytes += file.size;
        }
        next.insert(file.path, record(file, actual));
    }
    for (auto it = managed.begin(); it != managed.end(); ++it) {
        auto actual = oldActual[it.key()];
        if (isMod(it.key()) && !target.files.contains(it.key())) {
            require(!actual.endsWith(".disabled") || !it.value().toObject().value("identity").toString().isEmpty(),
                    "Cannot match a disabled mod across releases: " + actual + ". Restore its enabled state before updating.");
            p.removes.append(actual);
        }
        else if (!target.files.contains(it.key())) {
            auto path = safePath(root, actual);
            auto r = it.value().toObject();
            if (!protectedData(it.key()) && exists(path) && digest(path, requiredString(r, "algorithm")) == requiredString(r, "hash")) p.removes.append(actual);
            else p.skipped.append(actual);
        }
    }
    p.metadata = state(target, project, version, next);
    return p;
}
void verifyUnchanged(const QString &root, const Plan &p)
{
    QMap<QString, QString> now;
    for (auto path : tree(root)) now.insert(path, digest(safePath(root, path)));
    require(now == p.observed, "Instance files changed while preparing the update. Check again before installing.");
}

QStringList modIds(const QString &jar)
{
    QStringList ids;
    QuaZip zip(jar);
    require(zip.open(QuaZip::mdUnzip), "Cannot inspect mod archive: " + jar);
    for (auto name : {"fabric.mod.json", "quilt.mod.json", "mcmod.info", "META-INF/mods.toml", "META-INF/neoforge.mods.toml"}) {
        if (!zip.setCurrentFile(name)) continue;
        QuaZipFile file(&zip);
        require(file.open(QIODevice::ReadOnly) && file.size() < 4 * 1024 * 1024, "Cannot read mod metadata: " + jar);
        auto bytes = file.readAll();
        if (QString(name).endsWith(".toml")) {
            char error[256];
            auto table = toml_parse(bytes.data(), error, sizeof(error));
            require(table != nullptr, "Invalid TOML mod metadata: " + jar);
            auto mods = toml_array_in(table, "mods");
            if (mods) for (int i = 0; i < toml_array_nelem(mods); ++i) {
                auto mod = toml_table_at(mods, i);
                if (!mod) continue;
                auto id = toml_string_in(mod, "modId");
                if (id.ok) { ids.append(QString::fromUtf8(id.u.s)); free(id.u.s); }
            }
            toml_free(table);
        } else {
            auto doc = QJsonDocument::fromJson(bytes);
            if (QString(name) == "fabric.mod.json") ids.append(doc.object().value("id").toString());
            else if (QString(name) == "quilt.mod.json") ids.append(doc.object().value("quilt_loader").toObject().value("id").toString());
            else {
                auto list = doc.isArray() ? doc.array() : doc.object().value("modList").toArray();
                for (auto value : list) ids.append(value.toObject().value("modid").toString());
            }
        }
    }
    ids.removeAll(QString()); ids.removeDuplicates();
    return ids;
}
void validateMods(const QString &root, const QString &staged, const Plan &p)
{
    QMap<QString, QString> owners;
    QMap<QString, QString> jars;
    for (auto path : tree(root)) {
        if (path.startsWith("mods/") && path.endsWith(".jar") && !p.removes.contains(path) && !p.writes.contains(path)) jars.insert(path, safePath(root, path));
    }
    for (auto it = p.writes.begin(); it != p.writes.end(); ++it)
        if (it.key().startsWith("mods/") && it.key().endsWith(".jar")) jars.insert(it.key(), safePath(staged, it.key()));
    for (auto it = jars.begin(); it != jars.end(); ++it) {
        auto ids = modIds(it.value());
        require(!ids.isEmpty(), "Cannot establish mod identity: " + it.key() + ". Resolve this unrecognized mod before updating.");
        for (auto id : ids) {
            require(!owners.contains(id), "Duplicate mod ID " + id + ": " + owners.value(id) + " and " + it.key());
            owners.insert(id, it.key());
        }
    }
}

bool pending(const QString &root) { return exists(QDir(root).filePath(".pack-update/journal.json")); }
bool hasBackup(const QString &root) { return exists(QDir(root).filePath(".pack-update/previous.json")); }
void restore(const QString &root, const Progress &progress)
{
    auto stateDir = safePath(root, ".pack-update");
    auto journalPath = safePath(root, ".pack-update/journal.json");
    auto previous = safePath(root, ".pack-update/previous.json");
    const bool recovering = pending(root);
    auto journal = readJson(recovering ? journalPath : previous);
    require(journal.value("schema").toInt() == 1 && journal.value("paths").isObject(), "Invalid update recovery journal");
    const auto backupName = journal.value("backup").toString("backup");
    require(QRegularExpression("^backup(?:-[a-f0-9-]+)?$").match(backupName).hasMatch(), "Invalid backup location");
    auto backup = safePath(stateDir, backupName);
    auto paths = journal.value("paths").toObject();
    // Validate every backup before touching the instance. Repeated recovery is idempotent.
    for (auto it = paths.begin(); it != paths.end(); ++it) {
        safePath(root, it.key());
        require(it.key().compare(".pack-update", Qt::CaseInsensitive) != 0 && !it.key().startsWith(".pack-update/", Qt::CaseInsensitive), "Recovery cannot modify its own state");
        require(it.value().isString() && (it.value().toString().isEmpty() || QRegularExpression("^[a-f0-9]{128}$").match(it.value().toString()).hasMatch()), "Invalid recovery hash: " + it.key());
        if (!it.value().toString().isEmpty())
            require(digest(safePath(backup, it.key())) == it.value().toString(), "Backup is missing or damaged: " + it.key());
    }
    writeJson(journalPath, journal);
    for (auto it = paths.begin(); it != paths.end(); ++it) {
        if (progress) progress("Restoring " + it.key());
        auto path = safePath(root, it.key());
        if (it.value().toString().isEmpty()) require(!exists(path) || QFile::remove(path), "Cannot remove: " + path);
        else copyFile(safePath(backup, it.key()), path);
    }
    // A failed newer update must not discard the last successful restore point.
    if (!recovering || (exists(previous) && readJson(previous) == journal))
        require(!exists(previous) || QFile::remove(previous), "Cannot clear previous update record");
    require(QFile::remove(journalPath), "Cannot clear recovery journal");
}
void commit(const QString &root, const QString &staged, const QStringList &writes, const QStringList &removes, const Progress &progress, const std::function<qint64()> &spaceAvailable)
{
    require(!pending(root), "An interrupted update must be restored first");
    auto stateDir = safePath(root, ".pack-update");
    const auto backupName = "backup-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto backup = safePath(stateDir, backupName);
    auto tempBackup = safePath(root, ".pack-update/backup-new");
    auto all = tree(root, true);
    qint64 bytes = 0;
    for (auto path : all) bytes += QFileInfo(safePath(root, path)).size();
    QSet<QString> destinations;
    for (auto path : writes + removes) {
        safePath(root, path);
        require(path.compare(".pack-update", Qt::CaseInsensitive) != 0 && !path.startsWith(".pack-update/", Qt::CaseInsensitive), "Cannot update recovery files");
        require(!destinations.contains(path.toCaseFolded()), "Conflicting update paths: " + path);
        destinations.insert(path.toCaseFolded());
    }
    for (auto path : writes) {
        auto source = safePath(staged, path);
        require(QFileInfo(source).isFile(), "Missing staged file: " + path);
        bytes += QFileInfo(source).size();
    }
    QStorageInfo storage(root);
    auto available = spaceAvailable ? spaceAvailable() : storage.isValid() ? storage.bytesAvailable() : -1;
    require(available < 0 || available > bytes + 16 * 1024 * 1024, "Not enough space for the update backup");
    require(QDir(tempBackup).removeRecursively() && QDir().mkpath(tempBackup), "Cannot prepare backup");
    for (auto path : all) {
        if (progress) progress("Backing up " + path);
        copyFile(safePath(root, path), safePath(tempBackup, path));
    }
    // Ensure the snapshot is coherent before any mutation, including external edits during backup.
    require(tree(root, true) == all, "Instance files changed during backup");
    for (auto path : all)
        require(digest(safePath(root, path)) == digest(safePath(tempBackup, path)), "Instance file changed during backup: " + path);
    QJsonObject paths;
    auto affected = writes + removes;
    affected.removeDuplicates();
    for (auto path : affected) {
        require(!path.startsWith(".pack-update/"), "Cannot update recovery files");
        auto original = safePath(root, path);
        paths.insert(path, exists(original) ? digest(original) : QString());
    }
    // Unique snapshots keep the previous restore point valid until this commit succeeds.
    auto previous = safePath(root, ".pack-update/previous.json");
    require(QDir().rename(tempBackup, backup), "Cannot finalize backup");
    QJsonObject journal{{"schema", 1}, {"backup", backupName}, {"paths", paths}};
    auto journalPath = safePath(root, ".pack-update/journal.json");
    writeJson(journalPath, journal);
    try {
        for (auto path : removes) {
            if (progress) progress("Removing " + path);
            auto dest = safePath(root, path);
            require(!exists(dest) || QFile::remove(dest), "Cannot remove: " + path);
        }
        // Caller puts update metadata last. Every file replacement uses QSaveFile.
        for (auto path : writes) {
            if (progress) progress("Installing " + path);
            copyFile(safePath(staged, path), safePath(root, path));
        }
        writeJson(previous, journal);
        require(QFile::remove(journalPath), "Cannot finalize update journal");
    } catch (const Error &e) {
        try { restore(root, progress); }
        catch (const Error &recovery) { throw Error{e.message + "\nRecovery incomplete: " + recovery.message}; }
        throw;
    }
}
}
}
