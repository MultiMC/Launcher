#pragma once

#include <QAbstractListModel>
#include <QSet>
#include <QTimer>

#include <memory>
#include <vector>

class CustomTheme;
class ITheme;
class QFileSystemWatcher;

/**
 * The colour themes: the built-in ones, then one row per folder in themes/ (CustomTheme),
 * by name. Watches the folder and each theme's files: folders that come and go change the
 * rows, a changed theme.json or stylesheet reloads that theme (themeReloaded), so changes show
 * as they are saved. Broken themes have rows too, with the reason (ErrorRole), and can't be
 * chosen.
 */
class ThemeList : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles
    {
        IdRole = Qt::UserRole + 1,
        BuiltInRole,
        ErrorRole,
        DetailsRole // a line about it: who made it and its license, or why it can't be used
    };

    /// builtIn stay owned by the caller
    ThemeList(const QList<ITheme *> &builtIn, const QString &folder, QObject *parent = nullptr);
    virtual ~ThemeList();

    /// The theme with this id, if it can be used
    ITheme *theme(const QString &id) const;
    QModelIndex indexOf(const QString &id) const;
    QString folder() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

signals:
    /// A theme was read again (its files changed)
    void themeReloaded(const QString &id);

private:
    ITheme *themeAt(int row) const;
    CustomTheme *userTheme(const QString &id) const;
    void directoryChanged(const QString &path);
    void fileChanged(const QString &path);
    void processChanges();
    void rescan();
    void updateWatcher();

private: /* data */
    QList<ITheme *> m_builtIn;
    std::vector<std::unique_ptr<CustomTheme>> m_user;
    QString m_folder;
    QFileSystemWatcher *m_watcher = nullptr;
    // editors save in several steps: changes are collected for a moment, then handled once
    QTimer m_changes;
    bool m_rescanPending = false;
    QSet<QString> m_reloadPending;
};
