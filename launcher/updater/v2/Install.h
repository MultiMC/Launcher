#pragma once
#include <QFile>
#include <QList>
#include <QString>
#include <QStringList>

#include "Formats.h"

/**
 * Updater v2's apply step. It works from the target state, a build's variant: what has
 * to change on disk to get there is planned first (planInstall), then the Installer makes
 * the changes, moving everything it replaces or removes into a backup, so it can put it all
 * back. It is updater v2's own; updater v1 keeps its UpdateController.
 */
namespace UpdaterV2
{

/**
 * A content file to download: what it must be, and where it goes until it is installed.
 */
struct Download
{
    FileEntry file;
    QString target;
};

/**
 * One change to the installation, at a path relative to its root.
 */
struct Change
{
    enum Kind
    {
        // put the downloaded file `source` there
        Place,
        // make a symbolic link to `target` there
        Link,
        // take away what is there
        Remove,
        // the file there is right, only its executable bit isn't
        SetExecutable
    } kind = Place;
    QString path;
    QString source;
    QString target;
    bool executable = false;
};

/**
 * What it takes to go from what is installed under `root` to `next`:
 * - files of `next` that aren't there with the right content are downloaded into `stagingDir`
 *   and placed, files that only have the wrong executable bit get it fixed;
 * - links of `next` that aren't there pointing at the right target are made;
 * - files and links of `current` (the installed build's variant, when its manifest is known)
 *   that `next` doesn't have are removed.
 * Whatever is in the way of a file or a link (another kind of entry, a directory, a file that
 * can't be read) is replaced.
 * A file or link of `next` whose path differs only in case from one being removed is always
 * placed again: on a file system that ignores case, the removal takes away what is there.
 */
void planInstall(const Variant &next, const Variant *current, const QString &root, const QString &stagingDir,
                 QList<Download> &downloads, QList<Change> &changes);

/**
 * Get a backup directory in `updateDir` for the Installer: `backup`, after deleting the
 * previous update's backup in it. When that can't be deleted (a file in it held open, or
 * not allowed), a new empty `backup-XXXXXX` is used instead, so one stuck backup doesn't
 * stop every update after it; removeOldStaging() tries again later. Return an empty string
 * when there is no usable directory either way.
 */
QString prepareBackupDir(const QString &updateDir);

/**
 * Delete what earlier updates left in `updateDir`: every directory in it but `backup` and
 * `keep`, older backups that couldn't be deleted in time included. A download or an install that failed keeps its staging directory, a full copy of
 * the changed files each time; updater v1 uses the same directory.
 */
void removeOldStaging(const QString &updateDir, const QString &keep);

/**
 * Where a symbolic link points, exactly as it was made (Qt only gives absolute paths).
 * Return false when `path` isn't a link.
 */
bool readLink(const QString &path, QString &target);

/**
 * Makes planned changes under `root`. Everything it replaces or removes is moved to the same
 * path under `backupDir` (on the same file system, so moving is renaming), which must be
 * empty or not exist yet. A directory in the way that holds nothing but directories, as the
 * removals leave them, is deleted instead, and made again by rollback().
 */
class Installer
{
public:
    Installer(const QString &root, const QString &backupDir);

    /**
     * Make the changes in order. Stop at the first one that fails, and return false; the ones
     * made until then stay made, for rollback().
     */
    bool apply(const QList<Change> &changes);

    /**
     * Undo what apply() did, the last change first: placed files go back to where they came
     * from, links are removed, and what was there is moved back. Return false when something
     * couldn't be put back.
     */
    bool rollback();

    QString error() const
    {
        return m_error;
    }
    // the path of the change that failed
    QString failedPath() const
    {
        return m_failedPath;
    }

private:
    bool applyOne(const Change &change);
    bool backUp(const QString &path, QString &backup, QStringList &emptied);
    bool fail(const Change &change, const QString &error);

    struct Done
    {
        Change change;
        // where what was there went, empty when nothing was there
        QString backup;
        // the empty directories deleted instead, deepest first
        QStringList emptied;
        QFile::Permissions permissions;
    };

    QString m_root;
    QString m_backupDir;
    QList<Done> m_done;
    QString m_error;
    QString m_failedPath;
};

}
