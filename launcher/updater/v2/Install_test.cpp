#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include "FileSystem.h"
#include "updater/v2/Install.h"

using namespace UpdaterV2;

namespace {

FileEntry entry(const QString &path, const QByteArray &content, bool executable = false)
{
    FileEntry file;
    file.path = path;
    file.sha256 = QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex());
    file.size = content.size();
    file.executable = executable;
    return file;
}

LinkEntry link(const QString &path, const QString &target)
{
    LinkEntry out;
    out.path = path;
    out.target = target;
    return out;
}

void write(const QString &path, const QByteArray &content, bool executable = false)
{
    FS::ensureFilePathExists(path);
    QFile file(path);
    file.open(QFile::WriteOnly);
    file.write(content);
    file.close();
    auto permissions = QFile::permissions(path);
    auto bits = QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther;
    QFile::setPermissions(path, executable ? permissions | bits : permissions & ~bits);
}

QByteArray read(const QString &path)
{
    QFile file(path);
    file.open(QFile::ReadOnly);
    return file.readAll();
}

QString linkTarget(const QString &path)
{
    QString target;
    return readLink(path, target) ? target : QString();
}

// the installed build: a launcher, a library, a data file and (not on Windows) a framework link
Variant installed()
{
    Variant v;
    v.id = "linux-x86_64-qt5";
    v.files << entry("MultiMC", "launcher 1\n", true) << entry("lib/libfoo.so", "foo\n") << entry("share/old.txt", "old\n");
#ifndef Q_OS_WIN
    v.links << link("lib/libfoo.so.1", "libfoo.so");
#endif
    return v;
}

// the next build: a new launcher, the same library, another data file, the link moved
Variant next()
{
    Variant v;
    v.id = "linux-x86_64-qt5";
    v.files << entry("MultiMC", "launcher 2\n", true) << entry("lib/libfoo.so", "foo\n") << entry("share/new.txt", "new\n");
#ifndef Q_OS_WIN
    v.links << link("lib/current", "libfoo.so");
#endif
    return v;
}

void install(const QString &root, const Variant &variant)
{
    for(auto &file: variant.files)
    {
        auto content = file.path == "MultiMC" ? QByteArray("launcher 1\n") : file.path == "lib/libfoo.so" ? QByteArray("foo\n") : QByteArray("old\n");
        write(FS::PathCombine(root, file.path), content, file.executable);
    }
    for(auto &l: variant.links)
    {
        QFile::link(l.target, FS::PathCombine(root, l.path));
    }
}

// what the downloads would have written
void download(const QList<Download> &downloads)
{
    for(auto &d: downloads)
    {
        auto content = d.file.path == "MultiMC" ? QByteArray("launcher 2\n") : d.file.path == "share/new.txt" ? QByteArray("new\n") : QByteArray("foo\n");
        QCOMPARE(d.file.sha256, entry(d.file.path, content).sha256);
        write(d.target, content);
    }
}

// what the downloads would have written, by path
void download(const QList<Download> &downloads, const QMap<QString, QByteArray> &contents)
{
    for(auto &d: downloads)
    {
        QVERIFY(contents.contains(d.file.path));
        QCOMPARE(d.file.sha256, entry(d.file.path, contents[d.file.path]).sha256);
        write(d.target, contents[d.file.path]);
    }
}

// Windows has no executable bit (Qt says a file is executable by its name)
bool executableBit(const QFileInfo &info)
{
#ifdef Q_OS_WIN
    Q_UNUSED(info);
    return false;
#else
    return info.isExecutable();
#endif
}

// every entry under root: path -> content, or -> target for links
QMap<QString, QString> snapshot(const QString &root)
{
    QMap<QString, QString> out;
    QDir dir(root);
    QDirIterator it(root, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        auto path = it.next();
        QFileInfo info(path);
        auto relative = dir.relativeFilePath(path);
        QString target;
        if(readLink(path, target))
        {
            out.insert(relative, "-> " + target);
        }
        else if(info.isDir())
        {
            out.insert(relative, "/");
        }
        else
        {
            out.insert(relative, QString::fromUtf8(read(path)) + (executableBit(info) ? " (x)" : ""));
        }
    }
    return out;
}

}

class UpdaterV2InstallTest : public QObject
{
    Q_OBJECT

private
slots:
    void test_planFresh()
    {
        QTemporaryDir root, staging;
        QList<Download> downloads;
        QList<Change> changes;
        planInstall(next(), nullptr, root.path(), staging.path(), downloads, changes);
        QCOMPARE(downloads.size(), 3);
        QVERIFY(downloads[0].target.startsWith(staging.path()));
        QCOMPARE(changes[0].kind, Change::Place);
        QCOMPARE(changes[0].source, downloads[0].target);
        QVERIFY(changes[0].executable);
#ifndef Q_OS_WIN
        QCOMPARE(changes.size(), 4);
        QCOMPARE(changes[3].kind, Change::Link);
        QCOMPARE(changes[3].target, QString("libfoo.so"));
#endif
    }

    void test_planUpToDate()
    {
        QTemporaryDir root, staging;
        install(root.path(), installed());
        QList<Download> downloads;
        QList<Change> changes;
        auto current = installed();
        planInstall(installed(), &current, root.path(), staging.path(), downloads, changes);
        QCOMPARE(downloads.size(), 0);
        QCOMPARE(changes.size(), 0);
    }

    void test_planChanges()
    {
        QTemporaryDir root, staging;
        install(root.path(), installed());
        QList<Download> downloads;
        QList<Change> changes;
        auto current = installed();
        planInstall(next(), &current, root.path(), staging.path(), downloads, changes);
        // the library is the same, so only the launcher and the new data file are downloaded
        QCOMPARE(downloads.size(), 2);
        QStringList summary;
        for(auto &change: changes)
        {
            summary << QString("%1 %2").arg(change.kind).arg(change.path);
        }
#ifdef Q_OS_WIN
        QCOMPARE(summary, QStringList() << "2 share/old.txt" << "0 MultiMC" << "0 share/new.txt");
#else
        QCOMPARE(summary, QStringList() << "2 share/old.txt" << "2 lib/libfoo.so.1" << "0 MultiMC" << "0 share/new.txt" << "1 lib/current");
#endif
    }

    void test_planWithoutCurrent()
    {
        // without the installed build's manifest, nothing is removed
        QTemporaryDir root, staging;
        install(root.path(), installed());
        QList<Download> downloads;
        QList<Change> changes;
        planInstall(next(), nullptr, root.path(), staging.path(), downloads, changes);
        for(auto &change: changes)
        {
            QVERIFY(change.kind != Change::Remove);
        }
    }

#ifndef Q_OS_WIN
    void test_planExecutableBit()
    {
        QTemporaryDir root, staging;
        install(root.path(), installed());
        QFile::setPermissions(FS::PathCombine(root.path(), "MultiMC"), QFile::ReadOwner | QFile::WriteOwner);
        QList<Download> downloads;
        QList<Change> changes;
        planInstall(installed(), nullptr, root.path(), staging.path(), downloads, changes);
        QCOMPARE(downloads.size(), 0);
        QCOMPARE(changes.size(), 1);
        QCOMPARE(changes[0].kind, Change::SetExecutable);
        QVERIFY(changes[0].executable);
    }

    void test_planReplacesOtherKinds()
    {
        // a link where a file belongs, a file where a link belongs, a wrong link
        QTemporaryDir root, staging;
        QFile::link("libfoo.so", FS::PathCombine(root.path(), "MultiMC"));
        write(FS::PathCombine(root.path(), "lib/libfoo.so"), "foo\n");
        write(FS::PathCombine(root.path(), "share/new.txt"), "new\n");
        write(FS::PathCombine(root.path(), "lib/current"), "not a link\n");
        QList<Download> downloads;
        QList<Change> changes;
        planInstall(next(), nullptr, root.path(), staging.path(), downloads, changes);
        QCOMPARE(downloads.size(), 1);
        QCOMPARE(changes.size(), 2);
        QCOMPARE(changes[0].path, QString("MultiMC"));
        QCOMPARE(changes[1].kind, Change::Link);

        QFile::remove(FS::PathCombine(root.path(), "lib/current"));
        QFile::link("elsewhere", FS::PathCombine(root.path(), "lib/current"));
        planInstall(next(), nullptr, root.path(), staging.path(), downloads, changes);
        QCOMPARE(changes.last().kind, Change::Link);
    }
#endif

    void test_applyAndRollback()
    {
        QTemporaryDir root, staging, backups;
        install(root.path(), installed());
        auto before = snapshot(root.path());
        QList<Download> downloads;
        QList<Change> changes;
        auto current = installed();
        planInstall(next(), &current, root.path(), staging.path(), downloads, changes);
        download(downloads);

        auto backupDir = FS::PathCombine(backups.path(), "backup");
        Installer installer(root.path(), backupDir);
        QVERIFY2(installer.apply(changes), qPrintable(installer.error()));

        QMap<QString, QString> expected;
#ifdef Q_OS_WIN
        expected.insert("MultiMC", "launcher 2\n");
#else
        expected.insert("MultiMC", "launcher 2\n (x)");
#endif
        expected.insert("lib", "/");
        expected.insert("lib/libfoo.so", "foo\n");
        expected.insert("share", "/");
        expected.insert("share/new.txt", "new\n");
#ifndef Q_OS_WIN
        expected.insert("lib/current", "-> libfoo.so");
#endif
        QCOMPARE(snapshot(root.path()), expected);
        // what was replaced or removed is in the backup, at the same paths
        QCOMPARE(read(FS::PathCombine(backupDir, "MultiMC")), QByteArray("launcher 1\n"));
        QCOMPARE(read(FS::PathCombine(backupDir, "share/old.txt")), QByteArray("old\n"));
#ifndef Q_OS_WIN
        QCOMPARE(linkTarget(FS::PathCombine(backupDir, "lib/libfoo.so.1")), QString("libfoo.so"));
#endif

        QVERIFY(installer.rollback());
        QCOMPARE(snapshot(root.path()), before);
        // the downloaded files are back where they were
        QCOMPARE(read(downloads[0].target), QByteArray("launcher 2\n"));
    }

    void test_failureHalfway()
    {
        QTemporaryDir root, staging, backups;
        install(root.path(), installed());
        auto before = snapshot(root.path());
        QList<Download> downloads;
        QList<Change> changes;
        auto current = installed();
        planInstall(next(), &current, root.path(), staging.path(), downloads, changes);
        download(downloads);
        // the second download is missing
        QFile::remove(downloads[1].target);

        Installer installer(root.path(), FS::PathCombine(backups.path(), "backup"));
        QVERIFY(!installer.apply(changes));
        QCOMPARE(installer.failedPath(), QString("share/new.txt"));
        QVERIFY(installer.rollback());
        QCOMPARE(snapshot(root.path()), before);
    }

    void test_replacesDirectory()
    {
        // a directory where a file goes is moved out of the way, and back
        QTemporaryDir root, staging, backups;
        write(FS::PathCombine(root.path(), "MultiMC/stray.txt"), "stray\n");
        auto before = snapshot(root.path());
        Variant v;
        v.files << entry("MultiMC", "launcher 2\n", true);
        QList<Download> downloads;
        QList<Change> changes;
        planInstall(v, nullptr, root.path(), staging.path(), downloads, changes);
        download(downloads);
        Installer installer(root.path(), FS::PathCombine(backups.path(), "backup"));
        QVERIFY2(installer.apply(changes), qPrintable(installer.error()));
        QCOMPARE(read(FS::PathCombine(root.path(), "MultiMC")), QByteArray("launcher 2\n"));
        QVERIFY(installer.rollback());
        QCOMPARE(snapshot(root.path()), before);
    }

    void test_directoryBecomesFile()
    {
        // what the removals leave of a directory is in the way of a file of the same name
        QTemporaryDir root, staging, backups;
        Variant current;
        current.files << entry("a/b/c.txt", "c\n") << entry("a/d.txt", "d\n");
        write(FS::PathCombine(root.path(), "a/b/c.txt"), "c\n");
        write(FS::PathCombine(root.path(), "a/d.txt"), "d\n");
        auto before = snapshot(root.path());
        Variant v;
        v.files << entry("a", "a\n");
        QList<Download> downloads;
        QList<Change> changes;
        planInstall(v, &current, root.path(), staging.path(), downloads, changes);
        download(downloads, {{"a", "a\n"}});

        auto backupDir = FS::PathCombine(backups.path(), "backup");
        Installer installer(root.path(), backupDir);
        QVERIFY2(installer.apply(changes), qPrintable(installer.error()));
        QMap<QString, QString> expected;
        expected.insert("a", "a\n");
        QCOMPARE(snapshot(root.path()), expected);
        QCOMPARE(read(FS::PathCombine(backupDir, "a/b/c.txt")), QByteArray("c\n"));
        QCOMPARE(read(FS::PathCombine(backupDir, "a/d.txt")), QByteArray("d\n"));

        QVERIFY(installer.rollback());
        QCOMPARE(snapshot(root.path()), before);
    }

    void test_fileBecomesDirectory()
    {
        QTemporaryDir root, staging, backups;
        Variant current;
        current.files << entry("a", "a\n");
        write(FS::PathCombine(root.path(), "a"), "a\n");
        auto before = snapshot(root.path());
        Variant v;
        v.files << entry("a/b/c.txt", "c\n");
        QList<Download> downloads;
        QList<Change> changes;
        planInstall(v, &current, root.path(), staging.path(), downloads, changes);
        download(downloads, {{"a/b/c.txt", "c\n"}});

        Installer installer(root.path(), FS::PathCombine(backups.path(), "backup"));
        QVERIFY2(installer.apply(changes), qPrintable(installer.error()));
        QCOMPARE(read(FS::PathCombine(root.path(), "a/b/c.txt")), QByteArray("c\n"));

        // the directories made for the new file are in the way of the old one
        QVERIFY(installer.rollback());
        QCOMPARE(snapshot(root.path()), before);
    }

    void test_caseOnlyRename()
    {
        // Foo.txt becomes foo.txt and Lib/ becomes lib/, with the same content. A file system
        // that ignores case shows the old entries at the new paths too: both are written here.
        QTemporaryDir root, staging;
        Variant current;
        current.files << entry("Foo.txt", "foo\n") << entry("Lib/x.txt", "x\n") << entry("same.txt", "same\n");
        Variant v;
        v.files << entry("foo.txt", "foo\n") << entry("lib/x.txt", "x\n") << entry("same.txt", "same\n");
#ifndef Q_OS_WIN
        current.links << link("Lib/current", "x.txt");
        v.links << link("lib/current", "x.txt");
#endif
        for(auto *variant: {&current, &v})
        {
            for(auto &file: variant->files)
            {
                write(FS::PathCombine(root.path(), file.path), file.path.endsWith("x.txt") ? "x\n" : file.path == "same.txt" ? "same\n" : "foo\n");
            }
#ifndef Q_OS_WIN
            for(auto &l: variant->links)
            {
                QFile::link(l.target, FS::PathCombine(root.path(), l.path));
            }
#endif
        }
        QList<Download> downloads;
        QList<Change> changes;
        planInstall(v, &current, root.path(), staging.path(), downloads, changes);

        // the old ones are removed, and the new ones placed although they look right
        QStringList placed, removed, linked;
        for(auto &change: changes)
        {
            (change.kind == Change::Place ? placed : change.kind == Change::Remove ? removed : linked).append(change.path);
        }
        QCOMPARE(placed, QStringList({"foo.txt", "lib/x.txt"}));
        QCOMPARE(downloads.size(), 2);
#ifdef Q_OS_WIN
        QCOMPARE(removed, QStringList({"Foo.txt", "Lib/x.txt"}));
        QVERIFY(linked.isEmpty());
#else
        QCOMPARE(removed, QStringList({"Foo.txt", "Lib/x.txt", "Lib/current"}));
        QCOMPARE(linked, QStringList({"lib/current"}));
#endif
    }

    void test_prepareBackupDir()
    {
        QTemporaryDir update;
        auto backup = FS::PathCombine(update.path(), "backup");
        write(FS::PathCombine(backup, "MultiMC"), "launcher 1\n");
        // the previous backup is deleted, and its place used again
        QCOMPARE(prepareBackupDir(update.path()), backup);
        QVERIFY(!QFileInfo::exists(backup));

#ifndef Q_OS_WIN
        // a backup that can't be deleted: a file in a directory without write permission
        auto stuck = FS::PathCombine(backup, "lib");
        write(FS::PathCombine(stuck, "libfoo.so"), "foo\n");
        auto permissions = QFile::permissions(stuck);
        QVERIFY(QFile::setPermissions(stuck, QFile::ReadOwner | QFile::ExeOwner));
        auto fresh = prepareBackupDir(update.path());
        QFile::setPermissions(stuck, permissions);
        QVERIFY(!fresh.isEmpty());
        QVERIFY(fresh != backup);
        QCOMPARE(QFileInfo(fresh).absolutePath(), QFileInfo(update.path()).absoluteFilePath());
        QVERIFY(QFileInfo(fresh).isDir());
        QVERIFY(QDir(fresh).entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden).isEmpty());

        // the next download clears the new one away, and the stuck one once it can be deleted
        removeOldStaging(update.path(), QString());
        QVERIFY(!QFileInfo::exists(fresh));
        QCOMPARE(prepareBackupDir(update.path()), backup);
#endif
    }

    void test_removeOldStaging()
    {
        QTemporaryDir update;
        write(FS::PathCombine(update.path(), "backup/MultiMC"), "launcher 1\n");
        write(FS::PathCombine(update.path(), "aB3xYz/MultiMC"), "launcher 2\n");
        write(FS::PathCombine(update.path(), "Qw12er/lib/libfoo.so"), "foo\n");
        FS::ensureFolderPathExists(FS::PathCombine(update.path(), "current"));
        write(FS::PathCombine(update.path(), "update.vbs"), "script\n");
        removeOldStaging(update.path(), FS::PathCombine(update.path(), "current"));
        QMap<QString, QString> expected;
        expected.insert("backup", "/");
        expected.insert("backup/MultiMC", "launcher 1\n");
        expected.insert("current", "/");
        expected.insert("update.vbs", "script\n");
        QCOMPARE(snapshot(update.path()), expected);
    }

#ifndef Q_OS_WIN
    void test_danglingLink()
    {
        // a link that points nowhere is still replaced, and put back
        QTemporaryDir root, staging, backups;
        FS::ensureFolderPathExists(FS::PathCombine(root.path(), "lib"));
        QFile::link("gone", FS::PathCombine(root.path(), "lib/current"));
        write(FS::PathCombine(root.path(), "lib/libfoo.so"), "foo\n");
        auto before = snapshot(root.path());
        Variant v;
        v.files << entry("lib/libfoo.so", "foo\n");
        v.links << link("lib/current", "libfoo.so");
        QList<Download> downloads;
        QList<Change> changes;
        planInstall(v, nullptr, root.path(), staging.path(), downloads, changes);
        QCOMPARE(changes.size(), 1);
        Installer installer(root.path(), FS::PathCombine(backups.path(), "backup"));
        QVERIFY2(installer.apply(changes), qPrintable(installer.error()));
        QCOMPARE(linkTarget(FS::PathCombine(root.path(), "lib/current")), QString("libfoo.so"));
        QVERIFY(installer.rollback());
        QCOMPARE(snapshot(root.path()), before);
    }
#endif
};

QTEST_GUILESS_MAIN(UpdaterV2InstallTest)

#include "Install_test.moc"
