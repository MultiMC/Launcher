#include "DesktopIntegration.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <QStandardPaths>
#include <QStringList>

#include "Exception.h"
#include "FileSystem.h"

namespace {
// The Launcher's Wayland app id (organization domain multimc.org and the executable's name, as
// Qt makes it) and the name the packages use; the X11 window class is MultiMC
const QString ID = QStringLiteral("org.multimc.MultiMC");
const QString ROOT_KEY = QStringLiteral("X-MultiMC-Root=");

QString dataHome()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
}

QString entryPath()
{
    return dataHome() + "/applications/" + ID + ".desktop";
}

QString iconPath()
{
    return dataHome() + "/icons/hicolor/scalable/apps/" + ID + ".svg";
}

QString startScript(const QString &root)
{
    return root + "/MultiMC";
}

// A package's entry: in one of the system's data folders, not the user's
bool packaged()
{
    for (const QString &dir : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation))
    {
        if (QDir(dir) != QDir(dataHome()) && QFileInfo(dir + "/applications/" + ID + ".desktop").isFile())
            return true;
    }
    return false;
}

// Why this copy gets no entry, or nothing (the temporary folders: MainWindow's warning's)
QString unavailable(const QString &root)
{
    if (root.startsWith(QDir::tempPath()) || root.contains("/TempState/") || root.contains("Rar$", Qt::CaseInsensitive))
        return QObject::tr("This copy of MultiMC is in a temporary folder.");
    if (!QFileInfo(startScript(root)).isExecutable())
        return QObject::tr("This copy of MultiMC has no start script (%1).").arg(startScript(root));
    return QString();
}

// A desktop entry string value (the Desktop Entry Specification's escapes)
QString escaped(QString value)
{
    return value.replace("\\", "\\\\").replace("\n", "\\n").replace("\t", "\\t").replace("\r", "\\r");
}

QString unescaped(const QString &value)
{
    QString out;
    for (int i = 0; i < value.size(); i++)
    {
        if (value[i] == '\\' && i + 1 < value.size())
        {
            const QChar next = value[++i];
            out += next == 's' ? QChar(' ') : next == 'n' ? QChar('\n') : next == 't' ? QChar('\t') : next == 'r' ? QChar('\r') : next;
        }
        else
        {
            out += value[i];
        }
    }
    return out;
}

// An Exec argument: quoted, with the characters that need it escaped, then as a string value
QString execArgument(const QString &argument)
{
    QString quoted;
    for (const QChar c : argument)
    {
        if (c == '"' || c == '`' || c == '$' || c == '\\')
            quoted += '\\';
        quoted += c;
    }
    return escaped('"' + quoted + '"');
}

QByteArray entry(const QString &root, bool shown)
{
    QStringList lines = {
        "[Desktop Entry]",
        "Type=Application",
        "Name=MultiMC",
        "GenericName=Minecraft launcher",
        "Comment=Manage Minecraft instances with ease",
        "Exec=" + execArgument(startScript(root)),
        "TryExec=" + escaped(startScript(root)),
        "Icon=" + ID,
        "StartupWMClass=MultiMC",
        "StartupNotify=true",
        "Terminal=false",
        "Categories=Game;",
        "Keywords=game;Minecraft;",
        QString("NoDisplay=") + (shown ? "false" : "true"),
        ROOT_KEY + escaped(root),
    };
    return (lines.join("\n") + "\n").toUtf8();
}

// The entry there is: none (exists false), one a copy wrote (its root), or another (root empty)
struct Existing
{
    bool exists = false;
    QString root;
};

Existing existing()
{
    Existing found;
    QFile file(entryPath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return found;
    found.exists = true;
    for (const QString &line : QString::fromUtf8(file.readAll()).split('\n'))
    {
        if (line.startsWith(ROOT_KEY))
            found.root = unescaped(line.mid(ROOT_KEY.size()));
    }
    return found;
}

bool writeIfChanged(const QString &path, const QByteArray &data)
{
    if (QFileInfo(path).isFile() && FS::read(path) == data)
        return false;
    FS::write(path, data);
    return true;
}

bool write(const QString &root, bool shown)
{
    try
    {
        if (writeIfChanged(entryPath(), entry(root, shown)))
            qDebug() << "Applications menu entry for" << root << (shown ? "written" : "written, hidden from the menu");
    }
    catch (const Exception &e)
    {
        qWarning() << "Couldn't write the applications menu entry:" << e.cause();
        return false;
    }
    // the logo is the executable's resource (branding/logo.qrc): without it, an entry still
    try
    {
        writeIfChanged(iconPath(), FS::read(":/logo.svg"));
    }
    catch (const Exception &e)
    {
        qWarning() << "Couldn't write the applications menu icon:" << e.cause();
    }
    return true;
}
}

DesktopIntegration::Status DesktopIntegration::status(const QString &root)
{
    Status status;
    status.why = unavailable(root);
    if (!status.why.isEmpty())
        return status;
    if (packaged())
    {
        status.state = State::Packaged;
        return status;
    }
    const Existing found = existing();
    if (!found.exists || QDir(found.root) == QDir(root))
    {
        status.state = State::Ours;
        return status;
    }
    status.state = State::Other;
    status.other = found.root;
    return status;
}

void DesktopIntegration::update(const QString &root, bool shown)
{
    const QString why = unavailable(root);
    if (!why.isEmpty())
    {
        qDebug() << "No applications menu entry:" << why;
        return;
    }
    const Existing found = existing();
    if (packaged())
    {
        // the user's entry would hide the package's: remove one a copy wrote
        if (found.exists && !found.root.isEmpty())
        {
            QFile::remove(entryPath());
            QFile::remove(iconPath());
            qDebug() << "A package provides the applications menu entry: removed the one for" << found.root;
        }
        return;
    }
    if (found.exists && found.root.isEmpty())
    {
        qDebug() << "The applications menu entry" << entryPath() << "isn't one the Launcher wrote: left alone";
        return;
    }
    if (found.exists && QDir(found.root) != QDir(root) && QFileInfo(startScript(found.root)).exists())
    {
        qDebug() << "The applications menu entry is another copy's:" << found.root;
        return;
    }
    write(root, shown);
}

bool DesktopIntegration::takeOver(const QString &root, bool shown)
{
    if (!unavailable(root).isEmpty() || packaged())
        return false;
    return write(root, shown);
}
