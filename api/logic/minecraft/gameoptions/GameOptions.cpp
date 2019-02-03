#include "GameOptions.h"
#include "FileSystem.h"
#include <QDebug>
#include <QSaveFile>

namespace {
bool load(const QString& path, RawGameOptions & contents)
{
    contents.clear();
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
    {
        qWarning() << "Failed to read options file.";
        return false;
    }
    while(!file.atEnd())
    {
        auto line = file.readLine();
        if(line.endsWith('\n'))
        {
            line.chop(1);
        }
        auto separatorIndex = line.indexOf(':');
        if(separatorIndex == -1)
        {
            continue;
        }
        auto key = QString::fromUtf8(line.data(), separatorIndex);
        auto value = QString::fromUtf8(line.data() + separatorIndex + 1, line.size() - 1 - separatorIndex);
        qDebug() << "!!" << key << "!!";
        if(key == "version")
        {
            contents.version = value.toInt();
            continue;
        }
        contents.mapping[key] = value;
    }
    qDebug() << "Loaded" << path << "with version:" << contents.version;
    return true;
}
bool save(const QString& path, RawGameOptions& contents)
{
    QSaveFile out(path);
    if(!out.open(QIODevice::WriteOnly))
    {
        return false;
    }
    if(contents.version != 0)
    {
        QString versionLine = QString("version:%1\n").arg(contents.version);
        out.write(versionLine.toUtf8());
    }
    auto iter = contents.mapping.begin();
    while (iter != contents.mapping.end())
    {
        out.write(iter->first.toUtf8());
        out.write(":");
        out.write(iter->second.toUtf8());
        out.write("\n");
        iter++;
    }
    return out.commit();
}
}

GameOptions::GameOptions(const QString& path):
    path(path)
{
    reload();
}

QVariant GameOptions::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
    {
        return QAbstractListModel::headerData(section, orientation, role);
    }
    switch(section)
    {
        case 0:
            return tr("Key");
        case 1:
            return tr("Value");
        default:
            return QVariant();
    }
}

QVariant GameOptions::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();
    int column = index.column();

    if (row < 0 || row >= rowCount())
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
        if(column == 0)
        {
            return cookedOptions.items[row].id;
        }
        else
        {
            return cookedOptions.items[row].default_value;
        }
    default:
        return QVariant();
    }
    return QVariant();
}

int GameOptions::rowCount(const QModelIndex&) const
{
    return cookedOptions.items.size();
}

int GameOptions::columnCount(const QModelIndex&) const
{
    return 2;
}

bool GameOptions::isLoaded() const
{
    return loaded;
}

bool GameOptions::reload()
{
    beginResetModel();
    loaded = load(path, rawOptions);
    endResetModel();
    return loaded;
}

bool GameOptions::save()
{
    return ::save(path, rawOptions);
}
