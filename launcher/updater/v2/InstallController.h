#pragma once

#include <memory>

#include <QList>
#include <QString>

#include "Install.h"

class QWidget;

namespace UpdaterV2
{

/**
 * Installs a downloaded update into the running Launcher's installation and starts the new
 * build in its place, rolling everything back when either fails. Updater v2's copy of
 * what UpdateController does for updater v1, on top of the Installer.
 */
class InstallController
{
public:
    /**
     * Without `interactive` (the headless --update mode) nothing is asked or shown: a failure
     * always rolls back, and the process isn't quit; succeeded() and error() say how it went.
     */
    InstallController(QWidget *parent, const QString &root, const QString &stagingDir, const QList<Change> &changes,
                      bool interactive = true);
    void installUpdates();

    // the new build was installed and has started
    bool succeeded() const
    {
        return m_succeeded;
    }
    QString error() const
    {
        return m_error;
    }

private:
    // tell the user what failed and, if they want or it is needed, roll back; quits when nothing works
    void fail(const QString &message, bool ask);

    QWidget *m_parent;
    QString m_root;
    QString m_stagingDir;
    QList<Change> m_changes;
    bool m_interactive = true;
    bool m_succeeded = false;
    QString m_error;
    QString m_backupDir;
    // made once the backup directory is known
    std::unique_ptr<Installer> m_installer;
};

}
