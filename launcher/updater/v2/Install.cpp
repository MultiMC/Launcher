#include "Install.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSet>

#include "FileSystem.h"

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

namespace UpdaterV2
{

namespace {

const QFile::Permissions ExecutableBits = QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther;

// rw-r--r--, plus x for everyone when executable
QFile::Permissions filePermissions(bool executable)
{
    QFile::Permissions permissions = QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser
        | QFile::ReadGroup | QFile::ReadOther;
    if(executable)
    {
        permissions |= ExecutableBits;
    }
    return permissions;
}

// anything there, a link that points nowhere included
bool present(const QFileInfo &info)
{
    return info.exists() || info.isSymLink();
}

// Qt's renames check that the source exists, following links, so they refuse a link that
// points nowhere; and QFile::rename() copies when renaming fails, which copies what a link
// points to. rename() moves the entry itself. Windows has no links here.
bool move(const QString &from, const QString &to)
{
#ifdef Q_OS_WIN
    return QDir().rename(from, to);
#else
    return ::rename(QFile::encodeName(from).constData(), QFile::encodeName(to).constData()) == 0;
#endif
}

// an installed file that can't be read counts as different, and is replaced
bool sameContent(const QString &path, const FileEntry &file)
{
    QFile installed(path);
    if(installed.size() != file.size || !installed.open(QFile::ReadOnly))
    {
        return false;
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(&installed);
    return QString::fromLatin1(hash.result().toHex()) == file.sha256;
}

}

bool readLink(const QString &path, QString &target)
{
#ifdef Q_OS_WIN
    // Windows variants have no links
    Q_UNUSED(path);
    Q_UNUSED(target);
    return false;
#else
    // link targets in the manifests are at most 512 characters; a longer one is no link of ours
    char buffer[1024];
    auto size = ::readlink(QFile::encodeName(path).constData(), buffer, sizeof(buffer));
    if(size < 0 || size == ssize_t(sizeof(buffer)))
    {
        return false;
    }
    target = QFile::decodeName(QByteArray(buffer, int(size)));
    return true;
#endif
}

void planInstall(const Variant &next, const Variant *current, const QString &root, const QString &stagingDir,
                 QList<Download> &downloads, QList<Change> &changes)
{
    downloads.clear();
    changes.clear();

    QSet<QString> nextPaths;
    for(auto &file: next.files)
    {
        nextPaths.insert(file.path);
    }
    for(auto &link: next.links)
    {
        nextPaths.insert(link.path);
    }

    if(current)
    {
        QStringList currentPaths;
        for(auto &file: current->files)
        {
            currentPaths.append(file.path);
        }
        for(auto &link: current->links)
        {
            currentPaths.append(link.path);
        }
        for(auto &path: currentPaths)
        {
            if(!nextPaths.contains(path) && present(QFileInfo(FS::PathCombine(root, path))))
            {
                Change change;
                change.kind = Change::Remove;
                change.path = path;
                changes.append(change);
            }
        }
    }

    int number = 0;
    for(auto &file: next.files)
    {
        auto path = FS::PathCombine(root, file.path);
        QFileInfo info(path);
        if(info.isFile() && !info.isSymLink() && sameContent(path, file))
        {
#ifndef Q_OS_WIN
            // Windows has no executable bit: Qt says .exe files are executable, whatever the manifest says
            if(info.isExecutable() != file.executable)
            {
                Change change;
                change.kind = Change::SetExecutable;
                change.path = file.path;
                change.executable = file.executable;
                changes.append(change);
            }
#endif
            continue;
        }
        Download download;
        download.file = file;
        // every change has its own file, named so the log says what it is
        download.target = FS::PathCombine(stagingDir, QString("%1-%2").arg(number++).arg(file.path.section('/', -1)));
        downloads.append(download);

        Change change;
        change.kind = Change::Place;
        change.path = file.path;
        change.source = download.target;
        change.executable = file.executable;
        changes.append(change);
    }

    for(auto &link: next.links)
    {
        QString target;
        if(readLink(FS::PathCombine(root, link.path), target) && target == link.target)
        {
            continue;
        }
        Change change;
        change.kind = Change::Link;
        change.path = link.path;
        change.target = link.target;
        changes.append(change);
    }
}

Installer::Installer(const QString &root, const QString &backupDir) : m_root(root), m_backupDir(backupDir)
{
}

bool Installer::fail(const Change &change, const QString &error)
{
    qWarning() << "Updater:" << error;
    m_error = error;
    m_failedPath = change.path;
    return false;
}

bool Installer::backUp(const QString &path, QString &backup)
{
    backup.clear();
    auto full = FS::PathCombine(m_root, path);
    if(!present(QFileInfo(full)))
    {
        return true;
    }
    auto destination = FS::PathCombine(m_backupDir, path);
    if(!FS::ensureFilePathExists(destination) || !move(full, destination))
    {
        return false;
    }
    backup = destination;
    return true;
}

bool Installer::applyOne(const Change &change)
{
    auto full = FS::PathCombine(m_root, change.path);
    Done done;
    done.change = change;

    if(change.kind == Change::SetExecutable)
    {
        done.permissions = QFile::permissions(full);
        auto permissions = change.executable ? done.permissions | ExecutableBits : done.permissions & ~ExecutableBits;
        if(!QFile::setPermissions(full, permissions))
        {
            return fail(change, QString("Couldn't change the permissions of %1").arg(full));
        }
        m_done.append(done);
        return true;
    }

    if(!backUp(change.path, done.backup))
    {
        return fail(change, QString("Couldn't move %1 out of the way").arg(full));
    }
    // from here on, rollback() puts it back
    m_done.append(done);

    switch(change.kind)
    {
        case Change::Place:
        {
            if(!FS::ensureFilePathExists(full) || !move(change.source, full))
            {
                m_done.last().change.kind = Change::Remove;
                return fail(change, QString("Couldn't move %1 to %2").arg(change.source, full));
            }
            QFile::setPermissions(full, filePermissions(change.executable));
            return true;
        }
        case Change::Link:
        {
            if(!FS::ensureFilePathExists(full) || !QFile::link(change.target, full))
            {
                m_done.last().change.kind = Change::Remove;
                return fail(change, QString("Couldn't make the link %1 to %2").arg(full, change.target));
            }
            return true;
        }
        case Change::Remove:
        case Change::SetExecutable:
        default:
            return true;
    }
}

bool Installer::apply(const QList<Change> &changes)
{
    m_error.clear();
    m_failedPath.clear();
    for(auto &change: changes)
    {
        if(!applyOne(change))
        {
            return false;
        }
    }
    return true;
}

bool Installer::rollback()
{
    bool ok = true;
    while(!m_done.isEmpty())
    {
        auto done = m_done.takeLast();
        auto full = FS::PathCombine(m_root, done.change.path);
        switch(done.change.kind)
        {
            case Change::SetExecutable:
            {
                if(!QFile::setPermissions(full, done.permissions))
                {
                    qWarning() << "Updater: couldn't restore the permissions of" << full;
                    ok = false;
                }
                continue;
            }
            case Change::Place:
            {
                if(!move(full, done.change.source))
                {
                    qWarning() << "Updater: couldn't move" << full << "back to" << done.change.source;
                    ok = false;
                    continue;
                }
                break;
            }
            case Change::Link:
            {
                if(!QFile::remove(full))
                {
                    qWarning() << "Updater: couldn't remove the link" << full;
                    ok = false;
                    continue;
                }
                break;
            }
            case Change::Remove:
            default:
                break;
        }
        if(!done.backup.isEmpty() && !move(done.backup, full))
        {
            qWarning() << "Updater: couldn't restore" << full << "from" << done.backup;
            ok = false;
        }
    }
    return ok;
}

}
