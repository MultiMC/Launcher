#pragma once

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
    InstallController(QWidget *parent, const QString &root, const QString &stagingDir, const QList<Change> &changes);
    void installUpdates();

private:
    // tell the user what failed and, if they want or it is needed, roll back; quits when nothing works
    void fail(const QString &message, bool ask);

    QWidget *m_parent;
    QString m_root;
    QString m_stagingDir;
    QList<Change> m_changes;
    Installer m_installer;
};

}
