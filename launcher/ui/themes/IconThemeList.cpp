#include "IconThemeList.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QSettings>

#include <xdgicon.h>

#include <algorithm>

#include "FileSystem.h"
#include "ReuseInfo.h"

namespace {
// the built-in sets, by their setting value, in the order they are shown
const char *const BUILT_IN[] = {"multimc", "pe_colored", "pe_blue", "pe_dark", "pe_light", "flat", "OSX", "iOS"};
// what the selector shows of each
const char *const SAMPLES[] = {"new", "viewfolder", "settings", "help", "checkupdate", "news"};
const int SAMPLE_SIZE = 24;
}

IconThemeList::IconThemeList(const QString &folder, QObject *parent)
    : QAbstractListModel(parent), m_folder(QDir(folder).absolutePath())
{
    // there to drop icon themes into, and to watch
    FS::ensureFolderPathExists(m_folder);
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, &m_changes, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_watcher, &QFileSystemWatcher::fileChanged, &m_changes, static_cast<void (QTimer::*)()>(&QTimer::start));
    m_changes.setSingleShot(true);
    m_changes.setInterval(200);
    connect(&m_changes, &QTimer::timeout, this, &IconThemeList::rescan);
    rescan();
}

IconThemeList::~IconThemeList()
{
}

int IconThemeList::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant IconThemeList::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_entries.size())
        return QVariant();
    const Entry &entry = m_entries[index.row()];
    const bool builtIn = entry.name.isEmpty();
    switch (role)
    {
    case Qt::DisplayRole:
        if (!builtIn)
            return entry.name;
        if (entry.id == "multimc")
            return tr("Default");
        if (entry.id == "pe_colored")
            return tr("Colored");
        if (entry.id == "pe_blue")
            return tr("Blue");
        if (entry.id == "pe_dark")
            return tr("Dark icons");
        if (entry.id == "pe_light")
            return tr("Light icons");
        if (entry.id == "flat")
            return QString("Flat");
        return entry.id; // OSX, iOS
    case Qt::ToolTipRole:
        if (!entry.error.isEmpty())
            return tr("This icon theme can't be used: %1").arg(entry.error);
        return builtIn ? QVariant() : QVariant(QDir(m_folder).absoluteFilePath(entry.id));
    case IdRole:
        return entry.id;
    case BuiltInRole:
        return builtIn;
    case ErrorRole:
        return entry.error;
    case DetailsRole:
        if (builtIn)
            return tr("Built in");
        return entry.error.isEmpty() ? entry.details : entry.error;
    default:
        return QVariant();
    }
}

Qt::ItemFlags IconThemeList::flags(const QModelIndex &index) const
{
    if (!index.data(ErrorRole).toString().isEmpty())
        return Qt::NoItemFlags;
    return QAbstractListModel::flags(index);
}

QModelIndex IconThemeList::indexOf(const QString &id) const
{
    for (int row = 0; row < m_entries.size(); row++)
    {
        if (m_entries[row].id == id)
            return index(row);
    }
    return QModelIndex();
}

QList<QPixmap> IconThemeList::samples(const QModelIndex &index) const
{
    if (index.row() < 0 || index.row() >= m_entries.size())
        return {};
    return m_entries[index.row()].samples;
}

QList<QPixmap> IconThemeList::renderSamples(const QString &id) const
{
    // the icon loader has one theme at a time: switch to this one for a moment
    QList<QPixmap> out;
    const QString current = XdgIcon::themeName();
    XdgIcon::setThemeName(id);
    for (auto name : SAMPLES)
        out.append(XdgIcon::fromTheme(name).pixmap(SAMPLE_SIZE, SAMPLE_SIZE));
    XdgIcon::setThemeName(current);
    return out;
}

void IconThemeList::rescan()
{
    beginResetModel();
    m_entries.clear();
    QStringList builtIn;
    for (auto id : BUILT_IN)
    {
        builtIn.append(id);
        m_entries.append({id, QString(), QString(), renderSamples(id), QString()});
    }
    QList<Entry> user;
    const auto folders = QDir(m_folder).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (auto &folder : folders)
    {
        const QString id = folder.fileName();
        if (builtIn.contains(id))
        {
            qWarning() << "Icon theme folder" << id << "has a built-in icon theme's name, skipped";
            continue;
        }
        Entry entry{id, id, QString(), {}, QString()};
        const QString index = QDir(folder.absoluteFilePath()).absoluteFilePath("index.theme");
        if (!QFileInfo(index).isFile())
        {
            entry.error = tr("It has no index.theme file.");
        }
        else
        {
            QSettings settings(index, QSettings::IniFormat);
            entry.name = settings.value("Icon Theme/Name", id).toString();
            entry.samples = renderSamples(id);
            QStringList details;
            const QString comment = settings.value("Icon Theme/Comment").toString();
            if (!comment.isEmpty())
                details.append(comment);
            const QString reuse = ReuseInfo::read(index).summary();
            if (!reuse.isEmpty())
                details.append(reuse);
            entry.details = details.join(QString::fromUtf8(" \xC2\xB7 "));
        }
        user.append(entry);
    }
    std::sort(user.begin(), user.end(), [](const Entry &a, const Entry &b) {
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });
    m_entries.append(user);
    endResetModel();
    updateWatcher();
}

void IconThemeList::updateWatcher()
{
    QStringList paths = {m_folder};
    for (auto &entry : m_entries)
    {
        if (!entry.name.isEmpty())
            paths.append(QDir(m_folder).absoluteFilePath(entry.id));
    }
    const QStringList watched = m_watcher->directories();
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
