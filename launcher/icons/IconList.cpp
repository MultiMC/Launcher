/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "IconList.h"
#include "IconUtils.h"
#include <FileSystem.h>
#include <QMap>
#include <QEventLoop>
#include <QMimeData>
#include <QUrl>
#include <QFileSystemWatcher>
#include <QSet>
#include <QDebug>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QStyle>

#define MAX_SIZE 1024

IconList::IconList(const QStringList &builtinPaths, QString path, QObject *parent) : QAbstractListModel(parent)
{
    QSet<QString> builtinNames;
    m_logoIcon = QIcon(":/logo.svg");
    builtinNames.insert("logo");

    // add builtin icons
    for(auto & builtinPath: builtinPaths)
    {
        QDir instance_icons(builtinPath);
        auto file_info_list = instance_icons.entryInfoList(QDir::Files, QDir::Name);
        for (auto file_info : file_info_list)
        {
            builtinNames.insert(file_info.baseName());
        }
    }
    QStringList builtinList = builtinNames.toList();
    builtinList.sort();
    for(auto & builtinName : builtinList)
    {
        addThemeIcon(builtinName);
    }

    m_watcher.reset(new QFileSystemWatcher());
    is_watching = false;
    connect(m_watcher.get(), SIGNAL(directoryChanged(QString)),
            SLOT(directoryChanged(QString)));
    connect(m_watcher.get(), SIGNAL(fileChanged(QString)), SLOT(fileChanged(QString)));

    directoryChanged(path);
}

void IconList::directoryChanged(const QString &path)
{
    QDir new_dir (path);
    if(m_dir.absolutePath() != new_dir.absolutePath())
    {
        m_dir.setPath(path);
        m_dir.refresh();
        if(is_watching)
            stopWatching();
        startWatching();
    }
    if(!m_dir.exists())
        if(!FS::ensureFolderPathExists(m_dir.absolutePath()))
            return;
    m_dir.refresh();
    auto new_list = m_dir.entryList(QDir::Files, QDir::Name);
    for (auto it = new_list.begin(); it != new_list.end(); it++)
    {
        QString &foo = (*it);
        foo = m_dir.filePath(foo);
    }
    auto new_set = new_list.toSet();
    QList<QString> current_list;
    for (auto &it : icons)
    {
        if (!it.has(IconType::FileBased))
            continue;
        current_list.push_back(it.m_images[IconType::FileBased].filename);
    }
    QSet<QString> current_set = current_list.toSet();

    QSet<QString> to_remove = current_set;
    to_remove -= new_set;

    QSet<QString> to_add = new_set;
    to_add -= current_set;

    for (auto remove : to_remove)
    {
        qDebug() << "Removing " << remove;
        QFileInfo rmfile(remove);
        QString key = rmfile.baseName();
        int idx = getIconIndex(key);
        if (idx == -1)
            continue;
        icons[idx].remove(IconType::FileBased);
        if (icons[idx].type() == IconType::ToBeDeleted)
        {
            beginRemoveRows(QModelIndex(), idx, idx);
            icons.remove(idx);
            reindex();
            endRemoveRows();
        }
        else
        {
            dataChanged(index(idx), index(idx));
        }
        m_watcher->removePath(remove);
        notifyUpdated(key);
    }

    for (auto add : to_add)
    {
        qDebug() << "Adding " << add;
        QFileInfo addfile(add);
        QString key = addfile.baseName();
        if (addIcon(key, QString(), addfile.filePath(), IconType::FileBased))
        {
            m_watcher->addPath(add);
            notifyUpdated(key);
        }
    }
}

void IconList::fileChanged(const QString &path)
{
    qDebug() << "Checking " << path;
    QFileInfo checkfile(path);
    if (!checkfile.exists())
        return;
    QString key = checkfile.baseName();
    int idx = getIconIndex(key);
    if (idx == -1)
        return;
    QIcon icon = IconUtils::fromFile(path);
    if (icon.isNull())
        return;

    icons[idx].m_images[IconType::FileBased].icon = icon;
    dataChanged(index(idx), index(idx));
    notifyUpdated(key);
}

void IconList::SettingChanged(const Setting &setting, QVariant value)
{
    if(setting.id() != "IconsDir")
        return;

    directoryChanged(value.toString());
}

void IconList::startWatching()
{
    auto abs_path = m_dir.absolutePath();
    FS::ensureFolderPathExists(abs_path);
    is_watching = m_watcher->addPath(abs_path);
    if (is_watching)
    {
        qDebug() << "Started watching " << abs_path;
    }
    else
    {
        qDebug() << "Failed to start watching " << abs_path;
    }
}

void IconList::stopWatching()
{
    m_watcher->removePaths(m_watcher->files());
    m_watcher->removePaths(m_watcher->directories());
    is_watching = false;
}

QStringList IconList::mimeTypes() const
{
    QStringList types;
    types << "text/uri-list";
    return types;
}
Qt::DropActions IconList::supportedDropActions() const
{
    return Qt::CopyAction;
}

bool IconList::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (action == Qt::IgnoreAction)
        return true;
    // check if the action is supported
    if (!data || !(action & supportedDropActions()))
        return false;

    // files dropped from outside?
    if (data->hasUrls())
    {
        auto urls = data->urls();
        QStringList iconFiles;
        for (auto url : urls)
        {
            // only local files may be dropped...
            if (!url.isLocalFile())
                continue;
            iconFiles += url.toLocalFile();
        }
        installIcons(iconFiles);
        return true;
    }
    return false;
}

Qt::ItemFlags IconList::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    if (index.isValid())
        return Qt::ItemIsDropEnabled | defaultFlags;
    else
        return Qt::ItemIsDropEnabled | defaultFlags;
}

QVariant IconList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();

    if (row < 0 || row >= icons.size())
        return QVariant();

    switch (role)
    {
    case Qt::DecorationRole:
        return icons[row].icon();
    case Qt::DisplayRole:
        return icons[row].name();
    case Qt::UserRole:
        return icons[row].m_key;
    default:
        return QVariant();
    }
}

int IconList::rowCount(const QModelIndex &parent) const
{
    return icons.size();
}

void IconList::installIcons(const QStringList &iconFiles)
{
    for (QString file : iconFiles)
    {
        QFileInfo fileinfo(file);
        if (!fileinfo.isReadable() || !fileinfo.isFile())
            continue;
        // into the icons folder that's set, wherever it is (GH-2845)
        QString target = m_dir.filePath(fileinfo.fileName());

        QString suffix = fileinfo.suffix().toLower();
        if (suffix != "jpeg" && suffix != "png" && suffix != "jpg" && suffix != "ico" && suffix != "svg" && suffix != "gif")
            continue;

        if (!QFile::copy(file, target))
            continue;
    }
}

void IconList::installIcon(const QString &file, const QString &name)
{
    QFileInfo fileinfo(file);
    if(!fileinfo.isReadable() || !fileinfo.isFile())
        return;

    // an icon of this name stays as it is, whatever its file's extension
    if (iconFileExists(name))
        return;

    // named with the extension of what's in it, so other programs open it (GH-3702): downloads
    // come without one, or with the wrong one
    const QByteArray format = QImageReader::imageFormat(file);
    if (format.isEmpty())
    {
        qWarning() << "Icon" << file << "for" << name << "isn't an image MultiMC can read, not installed";
        return;
    }
    const QString extension = format == "jpeg" ? QStringLiteral("jpg") : QString::fromLatin1(format);
    QString target = m_dir.filePath(name + "." + extension);

    QFile::copy(file, target);
}

bool IconList::iconFileExists(const QString &key) const
{
    auto iconEntry = icon(key);
    if(!iconEntry)
    {
        return false;
    }
    return iconEntry->has(IconType::FileBased);
}

const MMCIcon *IconList::icon(const QString &key) const
{
    int iconIdx = getIconIndex(key);
    if (iconIdx == -1)
        return nullptr;
    return &icons[iconIdx];
}

bool IconList::deleteIcon(const QString &key)
{
    int iconIdx = getIconIndex(key);
    if (iconIdx == -1)
        return false;
    auto &iconEntry = icons[iconIdx];
    if (iconEntry.has(IconType::FileBased))
    {
        return QFile::remove(iconEntry.m_images[IconType::FileBased].filename);
    }
    return false;
}

bool IconList::addThemeIcon(const QString& key)
{
    auto iter = name_index.find(key);
    if (iter != name_index.end())
    {
        auto &oldOne = icons[*iter];
        oldOne.replace(Builtin, key);
        dataChanged(index(*iter), index(*iter));
        return true;
    }
    else
    {
        // add a new icon
        beginInsertRows(QModelIndex(), icons.size(), icons.size());
        {
            MMCIcon mmc_icon;
            mmc_icon.m_logoIcon = m_logoIcon;
            mmc_icon.m_name = key;
            mmc_icon.m_key = key;
            mmc_icon.replace(Builtin, key);
            icons.push_back(mmc_icon);
            name_index[key] = icons.size() - 1;
        }
        endInsertRows();
        return true;
    }
}

bool IconList::addIcon(const QString &key, const QString &name, const QString &path, const IconType type)
{
    // replace the icon even? is the input valid?
    QIcon icon = IconUtils::fromFile(path);
    if (icon.isNull())
        return false;
    auto iter = name_index.find(key);
    if (iter != name_index.end())
    {
        auto &oldOne = icons[*iter];
        oldOne.replace(type, icon, path);
        dataChanged(index(*iter), index(*iter));
        return true;
    }
    else
    {
        // add a new icon
        beginInsertRows(QModelIndex(), icons.size(), icons.size());
        {
            MMCIcon mmc_icon;
            mmc_icon.m_logoIcon = m_logoIcon;
            mmc_icon.m_name = name;
            mmc_icon.m_key = key;
            mmc_icon.replace(type, icon, path);
            icons.push_back(mmc_icon);
            name_index[key] = icons.size() - 1;
        }
        endInsertRows();
        return true;
    }
}

void IconList::saveIcon(const QString &key, const QString &path, const char * format) const
{
    // fitted in the middle of a transparent square: the macOS Dock squashes a wide image into
    // a square (GH-3820), and small ones come out as big as the others
    const QSize square(128, 128);
    const QPixmap fitted = IconUtils::fitted(getIcon(key), square, 1.0);
    if (fitted.isNull())
        return;
    QImage image(square, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.drawPixmap(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, fitted.size(), image.rect()), fitted);
    painter.end();
    image.save(path, format);
}


void IconList::reindex()
{
    name_index.clear();
    int i = 0;
    for (auto &iter : icons)
    {
        name_index[iter.m_key] = i;
        i++;
    }
}

QIcon IconList::getIcon(const QString &key) const
{
    int icon_index = getIconIndex(key);

    if (icon_index != -1)
        return icons[icon_index].icon();

    // Fallback for icons that don't exist.
    icon_index = getIconIndex("logo");

    if (icon_index != -1)
        return icons[icon_index].icon();
    return QIcon();
}

int IconList::getIconIndex(const QString &key) const
{
    auto iter = name_index.find(key == "default" ? defaultIconKey() : key);
    if (iter != name_index.end())
        return *iter;

    return -1;
}

QString IconList::getDirectory() const
{
    return m_dir.absolutePath();
}

//#include "IconList.moc"

void IconList::notifyUpdated(const QString &key)
{
    emit iconUpdated(key);
    if (!m_defaultKey.isEmpty() && key == m_defaultKey)
        emit iconUpdated("default");
}

QString IconList::defaultIconKey() const
{
    if (!m_defaultKey.isEmpty() && name_index.contains(m_defaultKey))
        return m_defaultKey;
    return "logo";
}

void IconList::setDefaultIcon(const QString &key)
{
    const QString newKey = (key == "default" || key == "logo") ? QString() : key;
    if (newKey == m_defaultKey)
        return;
    const int before = getIconIndex("default");
    m_defaultKey = newKey;
    const int after = getIconIndex("default");
    // the icons that stopped and started being the default, and everything showing "default"
    if (before != -1)
        emit dataChanged(index(before), index(before));
    if (after != -1)
        emit dataChanged(index(after), index(after));
    emit iconUpdated("default");
}

QString IconList::renameIcon(const QString &key, const QString &newKey)
{
    auto entry = icon(key);
    if (key == "default" || !entry || !entry->has(IconType::FileBased))
        return tr("Only icons from the icons folder can be renamed.");
    if (newKey == key)
        return QString();
    if (newKey.isEmpty())
        return tr("An icon needs a name.");
    // the key is the file's name up to the first dot; ':' is kept for namespaced keys; the rest
    // can't be in file names on Windows
    static const QString forbidden = QStringLiteral(".:/\\<>\"|?*");
    for (auto c : newKey)
    {
        if (forbidden.contains(c) || c.unicode() < 0x20)
            return tr("An icon's name can't contain any of these: %1").arg(". : / \\ < > \" | ? *");
    }
    if (newKey == "default" || newKey == "logo" || name_index.contains(newKey))
        return tr("There already is an icon called \"%1\".").arg(newKey);

    const QFileInfo file(entry->m_images[IconType::FileBased].filename);
    const QString target = m_dir.filePath(file.suffix().isEmpty() ? newKey : newKey + "." + file.suffix());
    if (QFileInfo::exists(target))
        return tr("There already is a file called \"%1\" in the icons folder.").arg(QFileInfo(target).fileName());
    if (!QFile::rename(file.filePath(), target))
        return tr("The file \"%1\" couldn't be renamed.").arg(file.fileName());

    // the list as the folder is now, without waiting for the watcher
    directoryChanged(m_dir.absolutePath());
    if (m_defaultKey == key)
        m_defaultKey = newKey;
    emit iconRenamed(key, newKey);
    return QString();
}
