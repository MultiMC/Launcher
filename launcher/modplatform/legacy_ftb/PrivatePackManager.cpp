#include "PrivatePackManager.h"

#include <QDebug>

#include "FileSystem.h"

namespace LegacyFTB {

void PrivatePackManager::load()
{
    try
    {
        auto data = QString::fromUtf8(FS::read(m_filename));
        auto list = data.split('\n', Qt::SkipEmptyParts);
        currentPacks = QSet<QString>(list.begin(), list.end());
        dirty = false;
    }
    catch(...)
    {
        currentPacks = {};
        qWarning() << "Failed to read third party FTB pack codes from" << m_filename;
    }
}

void PrivatePackManager::save() const
{
    if(!dirty)
    {
        return;
    }
    try
    {
        auto list = QStringList(currentPacks.begin(), currentPacks.end());
        FS::write(m_filename, list.join('\n').toUtf8());
        dirty = false;
    }
    catch(...)
    {
        qWarning() << "Failed to write third party FTB pack codes to" << m_filename;
    }
}

}
