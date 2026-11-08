#include "ThemeArchive.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>

#include <JlCompress.h>
#include <quazip.h>

#include "CustomTheme.h"
#include "FileSystem.h"
#include "MMCZip.h"

namespace {
const char *COLORS_FILE = "theme.json";
const char *ICONS_FILE = "index.theme";

QString tr(const char *text)
{
    return QCoreApplication::translate("ThemeArchive", text);
}

// The folders (empty for the top, else "name/") that hold the file, at most one level down
QStringList foldersWith(const QStringList &entries, const QString &file)
{
    QStringList out;
    for(auto &entry : entries)
    {
        if(entry == file)
        {
            out.append(QString());
        }
        else if(entry.endsWith('/' + file) && entry.count('/') == 1)
        {
            out.append(entry.left(entry.size() - file.size()));
        }
    }
    return out;
}
}

namespace ThemeArchive
{
nonstd::optional<Contents> inspect(const QString &zipPath, QString &error)
{
    QuaZip zip(zipPath);
    if(!zip.open(QuaZip::mdUnzip))
    {
        error = tr("It isn't a zip file, or it can't be read.");
        return nonstd::nullopt;
    }
    const QStringList entries = zip.getFileNameList();
    zip.close();

    const QStringList colors = foldersWith(entries, COLORS_FILE);
    const QStringList icons = foldersWith(entries, ICONS_FILE);
    if(colors.size() + icons.size() == 0)
    {
        error = tr("It has no theme.json (a theme) or index.theme (an icon theme), at the top or in a folder.");
        return nonstd::nullopt;
    }
    if(colors.size() + icons.size() > 1)
    {
        error = tr("It holds more than one theme. Import them one at a time.");
        return nonstd::nullopt;
    }

    Contents contents;
    contents.kind = colors.isEmpty() ? Kind::Icons : Kind::Colors;
    contents.subdir = colors.isEmpty() ? icons.first() : colors.first();
    contents.id = contents.subdir.isEmpty() ? QFileInfo(zipPath).completeBaseName() : contents.subdir.left(contents.subdir.size() - 1);
    if(contents.id.isEmpty() || contents.id.startsWith('.') || contents.id.contains('\\'))
    {
        error = tr("The theme's folder name \"%1\" can't be used.").arg(contents.id);
        return nonstd::nullopt;
    }
    return contents;
}

bool install(const QString &zipPath, const Contents &contents, const QString &folder, QString &error)
{
    const QString root = QDir(folder).absolutePath();
    const auto target = FS::PathInside(root, contents.id);
    if(!target || *target == root)
    {
        error = tr("The theme's folder name \"%1\" can't be used.").arg(contents.id);
        return false;
    }
    if(!QDir().mkpath(root))
    {
        error = tr("The folder %1 can't be created.").arg(root);
        return false;
    }

    // unpacked next to the themes, out of their list's sight, then moved in at once
    QTemporaryDir temp(QDir(QFileInfo(root).absolutePath()).absoluteFilePath(".theme-import-XXXXXX"));
    if(!temp.isValid())
    {
        error = tr("A temporary folder can't be created.");
        return false;
    }
    const QString staged = QDir(temp.path()).absoluteFilePath(contents.id);
    QuaZip zip(zipPath);
    if(!zip.open(QuaZip::mdUnzip) || !MMCZip::extractSubDirLimited(&zip, contents.subdir, staged, MAX_SIZE))
    {
        error = tr("It can't be unpacked: it is damaged, larger than %1 MB, or has files that would land outside the theme's folder.")
                    .arg(MAX_SIZE / (1024 * 1024));
        return false;
    }
    zip.close();

    if(contents.kind == Kind::Colors)
    {
        CustomTheme theme(staged);
        if(!theme.error().isEmpty())
        {
            error = theme.error();
            return false;
        }
    }

    if(QFileInfo::exists(*target) && !FS::deletePath(*target))
    {
        error = tr("The theme that is there can't be removed: %1").arg(*target);
        return false;
    }
    if(!QDir().rename(staged, *target))
    {
        error = tr("The theme can't be moved to %1.").arg(*target);
        return false;
    }
    qDebug() << "Imported theme" << zipPath << "to" << *target;
    return true;
}

bool exportTo(const QString &themeFolder, const QString &zipPath, QString &error)
{
    const QFileInfo folder(themeFolder);
    if(!JlCompress::compressDir(zipPath, folder.absoluteFilePath(), folder.fileName()))
    {
        QFile::remove(zipPath);
        error = tr("The zip file %1 can't be written.").arg(zipPath);
        return false;
    }
    return true;
}
}
