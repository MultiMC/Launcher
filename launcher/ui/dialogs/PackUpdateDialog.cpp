// SPDX-License-Identifier: Apache-2.0
#include "PackUpdateDialog.h"
#include "modplatform/modrinth/PackUpdate.h"
#include "Application.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/NetJob.h"
#include "net/ChecksumValidator.h"
#include "ProgressDialog.h"
#include <QtConcurrentRun>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QLockFile>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QVBoxLayout>

using namespace Modrinth::Update;
PackUpdateWork::PackUpdateWork(const QString &label, std::function<void()> work) : m_work(work)
{
    setStatus(label);
    setProgress(0, 0);
    connect(&m_watcher, &QFutureWatcher<QString>::finished, this, [this]() {
        auto error = m_watcher.result();
        if (error.isEmpty()) emitSucceeded(); else emitFailed(error);
    });
}
void PackUpdateWork::executeTask()
{
    m_watcher.setFuture(QtConcurrent::run([this]() -> QString {
        try { m_work(); return QString(); }
        catch (const Error &e) { return e.message; }
        catch (const std::exception &e) { return QString::fromUtf8(e.what()); }
    }));
}
namespace {
class UpdateProgress : public ProgressDialog
{
public:
    UpdateProgress(QWidget *parent, Task *task) : ProgressDialog(parent), m_task(task) {}
    void reject() override {
        if (m_task->isRunning()) {
            if (m_task->canAbort()) m_task->abort();
            return;
        }
        ProgressDialog::reject();
    }
private:
    Task *m_task;
};
void work(QWidget *parent, const QString &label, std::function<void()> fn)
{
    PackUpdateWork task(label, fn);
    UpdateProgress dialog(parent, &task);
    dialog.execWithTask(&task);
    if (!task.wasSuccessful()) throw Error{task.failReason()};
}
void download(QWidget *parent, NetJob &job)
{
    if (!job.size()) return;
    UpdateProgress dialog(parent, &job);
    dialog.setSkipButton(true, QObject::tr("Cancel"));
    dialog.execWithTask(&job);
    if (!job.wasSuccessful()) throw Error{job.failReason().isEmpty() ? QObject::tr("Download cancelled. No instance files were changed.") : job.failReason()};
}
QJsonDocument api(QWidget *parent, const QString &path)
{
    QByteArray bytes;
    NetJob job(QObject::tr("Checking Modrinth"), APPLICATION->network());
    job.addNetAction(Net::Download::makeByteArray(QUrl("https://api.modrinth.com/v2/" + path), &bytes));
    download(parent, job);
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError) throw Error{QObject::tr("Modrinth returned invalid JSON")};
    return doc;
}
QJsonArray apiVersions(QWidget *parent, const QString &project)
{
    auto document = api(parent, "project/" + project + "/version");
    if (!document.isArray()) throw Error{QObject::tr("Modrinth returned an invalid release list")};
    return document.array();
}
QString token(const QString &s)
{
    if (!QRegularExpression("^[A-Za-z0-9_-]+$").match(s).hasMatch()) throw Error{QObject::tr("Invalid Modrinth project/version ID")};
    return s;
}
QString cached(QWidget *parent, const QJsonObject &version, const QString &cache)
{
    auto files = version.value("files").toArray();
    QJsonObject selected;
    for (auto v : files) {
        auto f = v.toObject();
        if (!f.value("filename").toString().endsWith(".mrpack")) continue;
        if (selected.isEmpty() || f.value("primary").toBool()) selected = f;
        if (f.value("primary").toBool()) break;
    }
    if (selected.isEmpty()) throw Error{QObject::tr("This release has no Modrinth pack archive")};
    auto hash = selected.value("hashes").toObject().value("sha512").toString();
    if (!QRegularExpression("^[a-fA-F0-9]{128}$").match(hash).hasMatch()) throw Error{QObject::tr("Missing archive checksum")};
    hash = hash.toLower();
    auto path = safePath(cache, hash + ".mrpack");
    auto url = QUrl(selected.value("url").toString());
    if (url.scheme() != "https" || url.host().isEmpty() || !url.userInfo().isEmpty()) throw Error{QObject::tr("Invalid archive download URL")};
    if (QFileInfo::exists(path) && digest(path) == hash) return path;
    NetJob job(QObject::tr("Downloading modpack manifest and overrides"), APPLICATION->network());
    auto dl = Net::Download::makeFile(url, path);
    dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha512, QByteArray::fromHex(hash.toLatin1())));
    job.addNetAction(dl);
    download(parent, job);
    return path;
}
bool review(QWidget *parent, const QString &title, const QString &text, const QString &button)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    auto layout = new QVBoxLayout(&dialog);
    auto browser = new QTextBrowser(&dialog);
    browser->setPlainText(text);
    layout->addWidget(browser);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, &dialog);
    auto accept = buttons->addButton(button, QDialogButtonBox::AcceptRole);
    QObject::connect(accept, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.resize(700, 550);
    return dialog.exec() == QDialog::Accepted;
}
struct Busy {
    MinecraftInstance *instance;
    explicit Busy(MinecraftInstance *i) : instance(i) { i->setPackUpdateBusy(true); }
    ~Busy() { instance->setPackUpdateBusy(false); }
};
}
void PackUpdateDialog::run(QWidget *parent, MinecraftInstance *instance, bool restoreOnly)
{
    if (!instance || instance->isRunning() || instance->isLaunchPreparing() || instance->isPackUpdateBusy()) return;
    Busy busy(instance);
    try {
        auto root = instance->instanceRoot();
        auto game = instance->gameRoot();
        auto stateDir = safePath(root, ".pack-update");
        if (!QDir().mkpath(stateDir)) throw Error{QObject::tr("Cannot create updater directory")};
        QLockFile lock(safePath(root, ".pack-update/lock"));
        lock.setStaleLockTime(0);
        if (!lock.tryLock()) throw Error{QObject::tr("Another updater is using this instance")};
        auto profile = instance->getPackProfile();
        if (profile->getCurrentTask()) throw Error{QObject::tr("Wait for instance version loading to finish and try again.")};
        instance->saveNow();
        if (pending(root) || restoreOnly) {
            if (!pending(root) && !hasBackup(root)) throw Error{QObject::tr("There is no previous update to restore")};
            if (!review(parent, QObject::tr("Restore previous version"),
                        QObject::tr("Restore files changed by the last update? Files added by that update will be removed. Subsequent edits to those affected files will be replaced. Other settings, saves and user files remain untouched.\n\nA full pre-update snapshot is also retained in .pack-update (the journal identifies its backup directory). If a mod migrated your config after launching, that snapshot can be used for manual recovery."), QObject::tr("Restore"))) return;
            work(parent, QObject::tr("Restoring previous pack version"), [&]() { restore(root); });
            profile->reload(Net::Mode::Offline);
            QMessageBox::information(parent, QObject::tr("Update mods"), QObject::tr("Previous pack version restored."));
            return;
        }
        auto minecraft = profile->getComponentVersion("net.minecraft");
        QString loader;
        for (auto key : {"fabric-loader", "quilt-loader", "forge", "neoforge"}) {
            if (!profile->getComponentVersion(componentUid(key)).isEmpty()) {
                if (!loader.isEmpty()) throw Error{QObject::tr("Multiple loaders are not supported")};
                loader = key;
            }
        }
        auto metadataPath = safePath(root, "modrinth-update.json");
        auto cache = safePath(root, ".pack-update/cache");
        if (!QDir().mkpath(cache)) throw Error{QObject::tr("Cannot create download cache")};
        QTemporaryDir stage(stateDir + "/stage-XXXXXX");
        if (!stage.isValid()) throw Error{QObject::tr("Cannot create staging directory")};
        QJsonObject metadata;
        if (QFileInfo::exists(metadataPath)) metadata = readJson(metadataPath);
        if (metadata.value("project").toString().isEmpty() || metadata.value("version").toString().isEmpty()) {
            bool ok = false;
            auto project = QInputDialog::getText(parent, QObject::tr("Link existing modpack"),
                QObject::tr("Modrinth modpack URL or project ID:"), QLineEdit::Normal, QString(), &ok).trimmed();
            if (!ok) return;
            if (project.startsWith("https://modrinth.com/")) project = QUrl(project).path().section('/', 2, 2);
            project = token(project);
            auto projectInfo = api(parent, "project/" + project).object();
            if (projectInfo.value("project_type").toString() != "modpack") throw Error{QObject::tr("Select a Modrinth modpack project")};
            project = token(projectInfo.value("id").toString());
            auto versions = apiVersions(parent, project);
            QStringList labels;
            QList<QJsonObject> candidates;
            for (auto v : versions) {
                auto release = v.toObject();
                if (!release.value("game_versions").toArray().contains(minecraft)) continue;
                candidates.append(release);
                labels.append(release.value("version_number").toString() + " [" + release.value("id").toString() + "]");
            }
            if (labels.isEmpty()) throw Error{QObject::tr("No pack releases match this Minecraft version")};
            auto selected = QInputDialog::getItem(parent, QObject::tr("Installed pack release"),
                QObject::tr("Select the release originally installed (not the release you want to upgrade to):"), labels, 0, false, &ok);
            if (!ok) return;
            auto release = candidates[labels.indexOf(selected)];
            auto archive = cached(parent, release, cache);
            work(parent, QObject::tr("Verifying the original pack release"), [&]() {
                auto pack = readPack(archive, stage.path() + "/original");
                validateCompatibility(pack, minecraft, loader);
                metadata = baseline(pack, game, project, release.value("id").toString());
            });
            if (!review(parent, QObject::tr("Link modpack"),
                QObject::tr("Link this instance to %1, release %2?\n\nPack mods were checked against the original release hashes. Existing settings and extra mods will be preserved. Modified or missing pack mods must be restored before linking.")
                .arg(projectInfo.value("title").toString(), release.value("version_number").toString()), QObject::tr("Link"))) return;
            writeJson(metadataPath, metadata);
        }
        auto project = token(metadata.value("project").toString());
        auto current = api(parent, "version/" + token(metadata.value("version").toString())).object();
        if (current.value("project_id").toString() != project) throw Error{QObject::tr("Installed release belongs to a different project")};
        auto versions = apiVersions(parent, project);
        auto target = latestRelease(versions, minecraft, loader, current);
        if (target.isEmpty()) {
            bool newer = false;
            for (auto v : versions) if (v.toObject().value("version_type").toString() == "release" &&
                QDateTime::fromString(v.toObject().value("date_published").toString(), Qt::ISODate) > QDateTime::fromString(current.value("date_published").toString(), Qt::ISODate)) newer = true;
            QMessageBox::information(parent, QObject::tr("Update mods"), newer ? QObject::tr("No newer release for this Minecraft version and loader family.") : QObject::tr("Already up to date."));
            return;
        }
        if (target.value("project_id").toString() != project) throw Error{QObject::tr("Target release belongs to a different project")};
        token(target.value("id").toString());
        auto archive = cached(parent, target, cache);
        Pack pack;
        Plan changes;
        work(parent, QObject::tr("Comparing pack files and preserving your settings"), [&]() {
            pack = readPack(archive, stage.path() + "/overrides");
            validateCompatibility(pack, minecraft, loader);
            changes = plan(metadata, pack, game, project, target.value("id").toString());
        });
        auto deps = pack.manifest.value("dependencies").toObject();
        auto oldLoaderVersion = loader.isEmpty() ? QString() : profile->getComponentVersion(componentUid(loader));
        QString loaderNotice;
        if (!loader.isEmpty()) loaderNotice = QObject::tr("Loader: %1 → %2\n").arg(profile->getComponentVersion(componentUid(loader)), deps.value(loader).toString());
        // Account only for files not already present in the content-addressed cache.
        qint64 bytes = 0;
        work(parent, QObject::tr("Checking cached downloads"), [&]() {
            for (const auto &f : changes.writes) {
                if (f.overrideFile) continue;
                auto path = safePath(cache, f.algorithm + "-" + f.hash);
                if (!QFileInfo::exists(path) || digest(path, f.algorithm) != f.hash) bytes += f.size;
            }
        });
        auto summary = QObject::tr("Pack: %1 → %2\nMinecraft: %3 (unchanged)\n%4Download: %5 MiB (pack archive already fetched)\n\nAdd:\n%6\n\nReplace:\n%7\n\nRemove:\n%8\n\nPreserve existing settings/defaults:\n%9\n\nChangelog:\n%10\n\nExisting settings are preserved byte-for-byte. Updated mods may interpret them differently. A full backup is created before installation.")
            .arg(current.value("version_number").toString(), target.value("version_number").toString(), minecraft, loaderNotice)
            .arg(double(bytes) / (1024 * 1024), 0, 'f', 1)
            .arg(changes.added.join('\n'), changes.changed.join('\n'), changes.removes.join('\n'), changes.skipped.join('\n'), target.value("changelog").toString());
        if (!review(parent, QObject::tr("Update mods"), summary, QObject::tr("Update"))) return;
        auto gameRelative = QDir(root).relativeFilePath(game);
        auto payload = stage.path() + "/payload";
        auto stagedGame = safePath(payload, gameRelative);
        NetJob downloads(QObject::tr("Downloading updated pack files"), APPLICATION->network());
        for (const auto &f : changes.writes) {
            if (f.overrideFile) continue;
            auto path = safePath(cache, f.algorithm + "-" + f.hash);
            if (QFileInfo::exists(path) && digest(path, f.algorithm) == f.hash) continue;
            auto dl = Net::Download::makeFile(f.url, path);
            auto algo = f.algorithm == "sha512" ? QCryptographicHash::Sha512 : f.algorithm == "sha256" ? QCryptographicHash::Sha256 : QCryptographicHash::Sha1;
            dl->addValidator(new Net::ChecksumValidator(algo, QByteArray::fromHex(f.hash.toLatin1())));
            downloads.addNetAction(dl);
        }
        download(parent, downloads);
        // Freeze the profile to disk before its transactional replacement.
        instance->saveNow();
        work(parent, QObject::tr("Verifying, backing up and installing the update"), [&]() {
            verifyUnchanged(game, changes);
            QStringList writes, removes;
            for (const auto &f : changes.writes) {
                auto from = f.overrideFile ? safePath(stage.path() + "/overrides", f.path.endsWith(".disabled") ? f.path.left(f.path.size() - 9) : f.path) : safePath(cache, f.algorithm + "-" + f.hash);
                auto to = safePath(stagedGame, f.path);
                if (digest(from, f.algorithm) != f.hash || !QDir().mkpath(QFileInfo(to).absolutePath()) || !QFile::copy(from, to)) throw Error{QObject::tr("Cannot stage verified file: ") + f.path};
                writes.append(gameRelative + '/' + f.path);
            }
            validateMods(game, stagedGame, changes);
            for (auto path : changes.removes) removes.append(gameRelative + '/' + path);
            if (!loader.isEmpty() && oldLoaderVersion != deps.value(loader).toString()) {
                auto components = readJson(safePath(root, "mmc-pack.json"));
                if (QFileInfo::exists(safePath(root, "patches/" + componentUid(loader) + ".json"))) throw Error{QObject::tr("Restore the standard loader before updating a customized loader.")};
                components = updateLoaderComponent(components, loader, deps.value(loader).toString());
                writeJson(safePath(payload, "mmc-pack.json"), components);
                writes.append("mmc-pack.json");
            }
            writeJson(safePath(payload, "modrinth-update.json"), changes.metadata);
            writes.append("modrinth-update.json");
            commit(root, payload, writes, removes);
        });
        profile->reload(Net::Mode::Offline);
        QMessageBox::information(parent, QObject::tr("Update mods"), QObject::tr("Update installed. Your existing settings were preserved.\n\nUse Restore previous version to undo the pack update.") +
            (changes.skipped.isEmpty() ? QString() : QObject::tr("\n\nPreserved settings/defaults:\n") + changes.skipped.join('\n')));
    } catch (const Error &e) {
        // Disk commit rolls back on failure; reload the previous component profile as well.
        if (!pending(instance->instanceRoot())) instance->getPackProfile()->reload(Net::Mode::Offline);
        QMessageBox::warning(parent, QObject::tr("Update mods"), e.message);
    }
}
