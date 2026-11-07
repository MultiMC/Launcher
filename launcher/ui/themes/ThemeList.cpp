#include "ThemeList.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>

#include <algorithm>

#include "CustomTheme.h"
#include "FileSystem.h"

ThemeList::ThemeList(const QList<ITheme *> &builtIn, const QString &folder, QObject *parent)
    : QAbstractListModel(parent), m_builtIn(builtIn), m_folder(QDir(folder).absolutePath())
{
    // there to drop themes into, and to watch
    FS::ensureFolderPathExists(m_folder);
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &ThemeList::directoryChanged);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &ThemeList::fileChanged);
    m_changes.setSingleShot(true);
    m_changes.setInterval(200);
    connect(&m_changes, &QTimer::timeout, this, &ThemeList::processChanges);
    rescan();
}

ThemeList::~ThemeList()
{
}

QString ThemeList::folder() const
{
    return m_folder;
}

int ThemeList::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_builtIn.size() + int(m_user.size());
}

ITheme *ThemeList::themeAt(int row) const
{
    if (row < 0 || row >= rowCount())
        return nullptr;
    if (row < m_builtIn.size())
        return m_builtIn[row];
    return m_user[row - m_builtIn.size()].get();
}

QVariant ThemeList::data(const QModelIndex &index, int role) const
{
    ITheme *theme = themeAt(index.row());
    if (!theme)
        return QVariant();
    const bool builtIn = index.row() < m_builtIn.size();
    const QString error = builtIn ? QString() : static_cast<CustomTheme *>(theme)->error();
    switch (role)
    {
    case Qt::DisplayRole:
        return theme->name();
    case Qt::ToolTipRole:
        if (!error.isEmpty())
            return tr("This theme can't be used: %1").arg(error);
        return builtIn ? QVariant() : QVariant(QDir(m_folder).absoluteFilePath(theme->id()));
    case IdRole:
        return theme->id();
    case BuiltInRole:
        return builtIn;
    case ErrorRole:
        return error;
    case DetailsRole:
        if (builtIn)
            return tr("Built in");
        if (!error.isEmpty())
            return error;
        {
            const QString reuse = static_cast<CustomTheme *>(theme)->reuse().summary();
            return reuse.isEmpty() ? tr("In the themes folder, \"%1\"").arg(theme->id()) : reuse;
        }
    default:
        return QVariant();
    }
}

Qt::ItemFlags ThemeList::flags(const QModelIndex &index) const
{
    if (!index.data(ErrorRole).toString().isEmpty())
        return Qt::NoItemFlags;
    return QAbstractListModel::flags(index);
}

CustomTheme *ThemeList::userTheme(const QString &id) const
{
    for (auto &theme : m_user)
    {
        if (theme->id() == id)
            return theme.get();
    }
    return nullptr;
}

ITheme *ThemeList::theme(const QString &id) const
{
    for (auto theme : m_builtIn)
    {
        if (theme->id() == id)
            return theme;
    }
    CustomTheme *theme = userTheme(id);
    if (theme && theme->error().isEmpty())
        return theme;
    return nullptr;
}

QModelIndex ThemeList::indexOf(const QString &id) const
{
    for (int row = 0; row < rowCount(); row++)
    {
        if (themeAt(row)->id() == id)
            return index(row);
    }
    return QModelIndex();
}

void ThemeList::directoryChanged(const QString &path)
{
    // themes/ itself: folders came or went; a theme's folder: its files were replaced
    if (QDir(path) == QDir(m_folder))
        m_rescanPending = true;
    else
        m_reloadPending.insert(QFileInfo(path).fileName());
    m_changes.start();
}

void ThemeList::fileChanged(const QString &path)
{
    for (auto &theme : m_user)
    {
        if (theme->files().contains(path))
            m_reloadPending.insert(theme->id());
    }
    m_changes.start();
}

void ThemeList::processChanges()
{
    if (m_rescanPending)
    {
        m_rescanPending = false;
        m_reloadPending.clear();
        rescan();
        return;
    }
    const auto pending = m_reloadPending;
    m_reloadPending.clear();
    for (auto &id : pending)
    {
        CustomTheme *theme = userTheme(id);
        if (!theme)
            continue;
        qDebug() << "Theme" << id << "changed, reading it again";
        theme->reload();
        auto changed = indexOf(id);
        emit dataChanged(changed, changed);
        emit themeReloaded(id);
    }
    // editors replace files, which drops them from the watcher
    updateWatcher();
}

void ThemeList::rescan()
{
    beginResetModel();
    m_user.clear();
    QStringList builtInIds;
    for (auto theme : m_builtIn)
        builtInIds.append(theme->id());
    const auto folders = QDir(m_folder).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (auto &folder : folders)
    {
        if (builtInIds.contains(folder.fileName()))
        {
            qWarning() << "Theme folder" << folder.fileName() << "has a built-in theme's name, skipped";
            continue;
        }
        std::unique_ptr<CustomTheme> theme(new CustomTheme(folder.absoluteFilePath()));
        if (theme->isOldDefault())
            continue;
        m_user.push_back(std::move(theme));
    }
    std::sort(m_user.begin(), m_user.end(), [](const std::unique_ptr<CustomTheme> &a, const std::unique_ptr<CustomTheme> &b) {
        return a->name().compare(b->name(), Qt::CaseInsensitive) < 0;
    });
    endResetModel();
    qDebug() << "Themes in" << m_folder << ":" << m_user.size();
    updateWatcher();
    // every theme may have changed
    for (auto &theme : m_user)
        emit themeReloaded(theme->id());
}

void ThemeList::updateWatcher()
{
    QStringList paths = {m_folder};
    for (auto &theme : m_user)
    {
        paths.append(QFileInfo(theme->files().first()).absolutePath());
        for (auto &file : theme->files())
        {
            if (QFileInfo(file).exists())
                paths.append(file);
        }
    }
    const QStringList watched = m_watcher->files() + m_watcher->directories();
    QStringList gone;
    for (auto &path : watched)
    {
        if (!paths.contains(path))
            gone.append(path);
    }
    if (!gone.isEmpty())
        m_watcher->removePaths(gone);
    QStringList added;
    for (auto &path : paths)
    {
        if (!watched.contains(path))
            added.append(path);
    }
    if (!added.isEmpty())
        m_watcher->addPaths(added);
}
