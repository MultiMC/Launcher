#pragma once

#include <QString>
#include <QList>
#include <updater/v1/GoUpdate.h>

class QWidget;

namespace UpdaterV1
{

class UpdateController
{
public:
    UpdateController(QWidget * parent, const QString &root, const QString updateFilesDir, UpdaterV1::OperationList operations);
    void installUpdates();

private:
    void fail();
    bool rollback();

private:
    QString m_root;
    QString m_updateFilesDir;
    UpdaterV1::OperationList m_operations;
    QWidget * m_parent;

    struct BackupEntry
    {
        // path where we got the new file from
        QString update;
        // path of what is being actually updated
        QString original;
        // path where the backup of the updated file was placed
        QString backup;
    };
    QList <BackupEntry> m_replace_backups;
    QList <BackupEntry> m_delete_backups;
    enum Failure
    {
        Replace,
        Delete,
        Start,
        Nothing
    } m_failedOperationType = Nothing;
    QString m_failedFile;
};

}
