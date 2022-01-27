// Licensed under the Apache-2.0 license. See README.md for details.

#include "FileSystem.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QDebug>
#include <QUrl>
#include <QStandardPaths>
#include <QTextStream>

#if defined Q_OS_WIN32
    #include <windows.h>
    #include <string>
    #include <sys/utime.h>
    #include <winnls.h>
    #include <shobjidl.h>
    #include <objbase.h>
    #include <objidl.h>
    #include <shlguid.h>
    #include <shlobj.h>
#else
    #include <utime.h>
#endif

namespace FS {

void ensureExists(const QDir &dir)
{
    if (!QDir().mkpath(dir.absolutePath()))
    {
        throw FileSystemException("Unable to create folder " + dir.dirName() + " (" +
                                      dir.absolutePath() + ")");
    }
}

void write(const QString &filename, const QByteArray &data)
{
    ensureExists(QFileInfo(filename).dir());
    QSaveFile file(filename);
    if (!file.open(QSaveFile::WriteOnly))
    {
        throw FileSystemException("Couldn't open " + filename + " for writing: " +
                                  file.errorString());
    }
    if (data.size() != file.write(data))
    {
        throw FileSystemException("Error writing data to " + filename + ": " +
                                  file.errorString());
    }
    if (!file.commit())
    {
        throw FileSystemException("Error while committing data to " + filename + ": " +
                                  file.errorString());
    }
}

QByteArray read(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly))
    {
        throw FileSystemException("Unable to open " + filename + " for reading: " +
                                  file.errorString());
    }
    const qint64 size = file.size();
    QByteArray data(int(size), 0);
    const qint64 ret = file.read(data.data(), size);
    if (ret == -1 || ret != size)
    {
        throw FileSystemException("Error reading data from " + filename + ": " +
                                  file.errorString());
    }
    return data;
}

bool updateTimestamp(const QString& filename)
{
#ifdef Q_OS_WIN32
    std::wstring filename_utf_16 = filename.toStdWString();
    return (_wutime64(filename_utf_16.c_str(), nullptr) == 0);
#else
    QByteArray filenameBA = QFile::encodeName(filename);
    return (utime(filenameBA.data(), nullptr) == 0);
#endif
}

bool ensureFilePathExists(QString filenamepath)
{
    QFileInfo a(filenamepath);
    QDir dir;
    QString ensuredPath = a.path();
    bool success = dir.mkpath(ensuredPath);
    return success;
}

bool ensureFolderPathExists(QString foldernamepath)
{
    QFileInfo a(foldernamepath);
    QDir dir;
    QString ensuredPath = a.filePath();
    bool success = dir.mkpath(ensuredPath);
    return success;
}

bool copy::operator()(const QString &offset)
{
    //NOTE always deep copy on windows. the alternatives are too messy.
    #if defined Q_OS_WIN32
    m_followSymlinks = true;
    #endif

    auto src = PathCombine(m_src.absolutePath(), offset);
    auto dst = PathCombine(m_dst.absolutePath(), offset);

    QFileInfo currentSrc(src);
    if (!currentSrc.exists())
        return false;

    if(!m_followSymlinks && currentSrc.isSymLink())
    {
        qDebug() << "creating symlink" << src << " - " << dst;
        if (!ensureFilePathExists(dst))
        {
            qWarning() << "Cannot create path!";
            return false;
        }
        return QFile::link(currentSrc.symLinkTarget(), dst);
    }
    else if(currentSrc.isFile())
    {
        qDebug() << "copying file" << src << " - " << dst;
        if (!ensureFilePathExists(dst))
        {
            qWarning() << "Cannot create path!";
            return false;
        }
        return QFile::copy(src, dst);
    }
    else if(currentSrc.isDir())
    {
        qDebug() << "recursing" << offset;
        if (!ensureFolderPathExists(dst))
        {
            qWarning() << "Cannot create path!";
            return false;
        }
        QDir currentDir(src);
        for(auto & f : currentDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System))
        {
            auto inner_offset = PathCombine(offset, f);
            // ignore and skip stuff that matches the blacklist.
            if(m_blacklist && m_blacklist->matches(inner_offset))
            {
                continue;
            }
            if(!operator()(inner_offset))
            {
                qWarning() << "Failed to copy" << inner_offset;
                return false;
            }
        }
    }
    else
    {
        qCritical() << "Copy ERROR: Unknown filesystem object:" << src;
        return false;
    }
    return true;
}

bool deletePath(QString path)
{
    bool OK = true;
    QFileInfo finfo(path);
    if(finfo.isFile()) {
        return QFile::remove(path);
    }

    QDir dir(path);

    if (!dir.exists())
    {
        return OK;
    }
    auto allEntries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden |
                                        QDir::AllDirs | QDir::Files,
                                        QDir::DirsFirst);

    for(auto & info: allEntries)
    {
#if defined Q_OS_WIN32
        QString nativePath = QDir::toNativeSeparators(info.absoluteFilePath());
        auto wString = nativePath.toStdWString();
        DWORD dwAttrs = GetFileAttributesW(wString.c_str());
        // Windows: check for junctions, reparse points and other nasty things of that sort
        if(dwAttrs & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            if (info.isFile())
            {
                OK &= QFile::remove(info.absoluteFilePath());
            }
            else if (info.isDir())
            {
                OK &= dir.rmdir(info.absoluteFilePath());
            }
        }
#else
        // We do not trust Qt with reparse points, but do trust it with unix symlinks.
        if(info.isSymLink())
        {
            OK &= QFile::remove(info.absoluteFilePath());
        }
#endif
        else if (info.isDir())
        {
            OK &= deletePath(info.absoluteFilePath());
        }
        else if (info.isFile())
        {
            OK &= QFile::remove(info.absoluteFilePath());
        }
        else
        {
            OK = false;
            qCritical() << "Delete ERROR: Unknown filesystem object:" << info.absoluteFilePath();
        }
    }
    OK &= dir.rmdir(dir.absolutePath());
    return OK;
}


QString PathCombine(const QString & path1, const QString & path2)
{
    if(!path1.size())
        return path2;
    if(!path2.size())
        return path1;
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}

QString PathCombine(const QString & path1, const QString & path2, const QString & path3)
{
    return PathCombine(PathCombine(path1, path2), path3);
}

QString PathCombine(const QString & path1, const QString & path2, const QString & path3, const QString & path4)
{
    return PathCombine(PathCombine(path1, path2, path3), path4);
}

QString AbsolutePath(QString path)
{
    return QFileInfo(path).absolutePath();
}

QString ResolveExecutable(QString path)
{
    if (path.isEmpty())
    {
        return QString();
    }
    if(!path.contains('/'))
    {
        path = QStandardPaths::findExecutable(path);
    }
    QFileInfo pathInfo(path);
    if(!pathInfo.exists() || !pathInfo.isExecutable())
    {
        return QString();
    }
    return pathInfo.absoluteFilePath();
}

/**
 * Normalize path
 *
 * Any paths inside the current folder will be normalized to relative paths (to current)
 * Other paths will be made absolute
 */
QString NormalizePath(QString path)
{
    QDir a = QDir::currentPath();
    QString currentAbsolute = a.absolutePath();

    QDir b(path);
    QString newAbsolute = b.absolutePath();

    if (newAbsolute.startsWith(currentAbsolute))
    {
        return a.relativeFilePath(newAbsolute);
    }
    else
    {
        return newAbsolute;
    }
}

QString badFilenameChars = "\"\\/?<>:;*|!+\r\n";

QString RemoveInvalidFilenameChars(QString string, QChar replaceWith)
{
    for (int i = 0; i < string.length(); i++)
    {
        if (badFilenameChars.contains(string[i]))
        {
            string[i] = replaceWith;
        }
    }
    return string;
}

QString DirNameFromString(QString string, QString inDir)
{
    int num = 0;
    QString baseName = RemoveInvalidFilenameChars(string, '-');
    QString dirName;
    do
    {
        if(num == 0)
        {
            dirName = baseName;
        }
        else
        {
            dirName = baseName + QString::number(num);;
        }

        // If it's over 9000
        if (num > 9000)
            return "";
        num++;
    } while (QFileInfo(PathCombine(inDir, dirName)).exists());
    return dirName;
}

// Does the folder path contain any '!'? If yes, return true, otherwise false.
// (This is a problem for Java)
bool checkProblemticPathJava(QDir folder)
{
    QString pathfoldername = folder.absolutePath();
    return pathfoldername.contains("!", Qt::CaseInsensitive);
}

// Win32 crap
#if defined Q_OS_WIN

bool called_coinit = false;

HRESULT CreateLink(LPCSTR linkPath, LPCSTR targetPath, LPCSTR args)
{
    HRESULT hres;

    if (!called_coinit)
    {
        hres = CoInitialize(NULL);
        called_coinit = true;

        if (!SUCCEEDED(hres))
        {
            qWarning("Failed to initialize COM. Error 0x%08lX", hres);
            return hres;
        }
    }

    IShellLink *link;
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink,
                            (LPVOID *)&link);

    if (SUCCEEDED(hres))
    {
        IPersistFile *persistFile;

        link->SetPath(targetPath);
        link->SetArguments(args);

        hres = link->QueryInterface(IID_IPersistFile, (LPVOID *)&persistFile);
        if (SUCCEEDED(hres))
        {
            WCHAR wstr[MAX_PATH];

            MultiByteToWideChar(CP_ACP, 0, linkPath, -1, wstr, MAX_PATH);

            hres = persistFile->Save(wstr, TRUE);
            persistFile->Release();
        }
        link->Release();
    }
    return hres;
}

#endif

QString getDesktopDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
}

// Cross-platform Shortcut creation
bool createShortCut(QString location, QString dest, QStringList args, QString name,
                          QString icon)
{
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    location = PathCombine(location, name + ".desktop");

    QFile f(location);
    f.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream stream(&f);

    QString argstring;
    if (!args.empty())
        argstring = " '" + args.join("' '") + "'";

    stream << "[Desktop Entry]"
           << "\n";
    stream << "Type=Application"
           << "\n";
    stream << "TryExec=" << dest.toLocal8Bit() << "\n";
    stream << "Exec=" << dest.toLocal8Bit() << argstring.toLocal8Bit() << "\n";
    stream << "Name=" << name.toLocal8Bit() << "\n";
    stream << "Icon=" << icon.toLocal8Bit() << "\n";

    stream.flush();
    f.close();

    f.setPermissions(f.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup |
                     QFileDevice::ExeOther);

    return true;
#elif defined Q_OS_WIN
    // TODO: Fix
    //    QFile file(PathCombine(location, name + ".lnk"));
    //    WCHAR *file_w;
    //    WCHAR *dest_w;
    //    WCHAR *args_w;
    //    file.fileName().toWCharArray(file_w);
    //    dest.toWCharArray(dest_w);

    //    QString argStr;
    //    for (int i = 0; i < args.count(); i++)
    //    {
    //        argStr.append(args[i]);
    //        argStr.append(" ");
    //    }
    //    argStr.toWCharArray(args_w);

    //    return SUCCEEDED(CreateLink(file_w, dest_w, args_w));
    return false;
#else
    qWarning("Desktop Shortcuts not supported on your platform!");
    return false;
#endif
}
}
