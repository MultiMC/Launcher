#pragma once

#include <QAbstractListModel>
#include <QPixmap>
#include <QTimer>

class QFileSystemWatcher;

/**
 * The icon themes: the built-in sets, then one row per folder in iconthemes/ with an
 * index.theme (freedesktop icon themes, named by its Name=), by name. Watches the folder, so
 * themes that come and go show up at once. Each row has a few of its icons (samples()) for
 * the selector. Broken themes have rows too, with the reason (ErrorRole), and can't be chosen.
 */
class IconThemeList : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles
    {
        IdRole = Qt::UserRole + 1,
        BuiltInRole,
        ErrorRole,
        DetailsRole // a line about it: its description, who made it, its license
    };

    explicit IconThemeList(const QString &folder, QObject *parent = nullptr);
    virtual ~IconThemeList();

    QModelIndex indexOf(const QString &id) const;
    /// A few of the theme's icons, as the selector shows them
    QList<QPixmap> samples(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    struct Entry
    {
        QString id;
        QString name; // empty for the built-in ones: data() translates theirs
        QString error;
        QList<QPixmap> samples;
        QString details;
    };
    void rescan();
    QList<QPixmap> renderSamples(const QString &id) const;
    void updateWatcher();

private: /* data */
    QString m_folder;
    QList<Entry> m_entries;
    QFileSystemWatcher *m_watcher = nullptr;
    // folders are copied in several steps: changes are collected for a moment, then handled once
    QTimer m_changes;
};
