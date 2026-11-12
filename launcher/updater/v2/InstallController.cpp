#include "InstallController.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QTextStream>

#include "BuildConfig.h"
#include "FileSystem.h"
#include "updater/LiveCheck.h"

namespace UpdaterV2
{

namespace {
const QLatin1String liveCheckFile("live.check");
}

InstallController::InstallController(QWidget *parent, const QString &root, const QString &stagingDir, const QList<Change> &changes,
                                     bool interactive)
    : m_parent(parent), m_root(root), m_stagingDir(stagingDir), m_changes(changes), m_interactive(interactive)
{
}

void InstallController::installUpdates()
{
    qDebug() << "Updater: installing" << m_changes.size() << "changes";
#ifdef Q_OS_WIN
    QString finishCmd = QApplication::applicationFilePath();
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    QString finishCmd = FS::PathCombine(m_root, BuildConfig.LAUNCHER_NAME);
#elif defined Q_OS_MAC
    QString finishCmd = QApplication::applicationFilePath();
#else
#error Unsupported operating system.
#endif

    m_backupDir = prepareBackupDir(FS::PathCombine(m_root, "update"));
    if(m_backupDir.isEmpty())
    {
        fail(QObject::tr("Couldn't make a folder for the backup of this update in:\n%1").arg(FS::PathCombine(m_root, "update")), false);
        return;
    }
    m_installer.reset(new Installer(m_root, m_backupDir));

    QList<Change> changes;
    bool useXPHack = false;
    QString exePath;
    QString exeOrigin;
    QString exeBackup;
    for(auto change: m_changes)
    {
#ifdef Q_OS_WIN32
        QString windowsExeName = BuildConfig.LAUNCHER_NAME + ".exe";
        if(change.kind == Change::Place && change.path == windowsExeName)
        {
            // for people who renamed the .exe
            change.path = QFileInfo(QApplication::applicationFilePath()).fileName();
            // Windows XP can't move a running .exe: a script does it after this process quits
            if(QSysInfo::windowsVersion() < QSysInfo::WV_VISTA)
            {
                QDir rootDir(m_root);
                exeOrigin = rootDir.relativeFilePath(change.source);
                exePath = change.path;
                exeBackup = rootDir.relativeFilePath(FS::PathCombine(m_backupDir, change.path));
                useXPHack = true;
                continue;
            }
        }
#endif
        changes.append(change);
    }

    if(!m_installer->apply(changes))
    {
        fail(QObject::tr("Couldn't update %1. Changes will be reverted.\nSee the %2 log file for details.")
                 .arg(m_installer->failedPath(), BuildConfig.LAUNCHER_NAME), false);
        return;
    }

    // try to start the new binary
    auto args = qApp->arguments();
    args.removeFirst();

    // on old Windows, do insane things... no error checking here, this is just to have something.
    if(useXPHack)
    {
        QString script;
        auto nativePath = QDir::toNativeSeparators(exePath);
        auto nativeOriginPath = QDir::toNativeSeparators(exeOrigin);
        auto nativeBackupPath = QDir::toNativeSeparators(exeBackup);

        // so we write this vbscript thing...
        QTextStream out(&script);
        out << "WScript.Sleep 1000\n";
        out << "Set fso=CreateObject(\"Scripting.FileSystemObject\")\n";
        out << "Set shell=CreateObject(\"WScript.Shell\")\n";
        out << "fso.MoveFile \"" << nativePath << "\", \"" << nativeBackupPath << "\"\n";
        out << "fso.MoveFile \"" << nativeOriginPath << "\", \"" << nativePath << "\"\n";
        out << "shell.Run \"" << nativePath << "\"\n";

        // the script moves the old .exe into the backup
        FS::ensureFolderPathExists(m_backupDir);
        QString scriptPath = FS::PathCombine(m_root, "update", "update.vbs");

        // we save it
        QFile scriptFile(scriptPath);
        scriptFile.open(QIODevice::WriteOnly);
        scriptFile.write(script.toLocal8Bit().replace("\n", "\r\n"));
        scriptFile.close();

        // we run it
        QProcess::startDetached("wscript", {scriptPath}, m_root);

        // and we quit. conscious thought.
        qApp->quit();
        return;
    }

    bool doLiveCheck = true;
    bool startFailed = false;

    // remove live check file, if any
    if(QFile::exists(liveCheckFile))
    {
        if(!QFile::remove(liveCheckFile))
        {
            qWarning() << "Couldn't remove the" << liveCheckFile << "file! We will proceed without :(";
            doLiveCheck = false;
        }
    }

    if(doLiveCheck)
    {
        if(!args.contains("--alive"))
        {
            args.append("--alive");
        }
    }

    // a workaround for GH-1874: leave out the instance to launch
    QStringList realargs;
    int skip = 0;
    for(auto &arg: args)
    {
        if(skip)
        {
            skip--;
            continue;
        }
        if(arg == "-l")
        {
            skip = 1;
            continue;
        }
        realargs.append(arg);
    }

    // start the updated application
    qint64 pid = -1;
    bool started = QProcess::startDetached(finishCmd, realargs, QDir::currentPath(), &pid);
    if(!started || pid == -1)
    {
        qWarning() << "Couldn't start new process properly!";
        startFailed = true;
    }
    if(!startFailed && doLiveCheck)
    {
        startFailed = !LiveCheck::waitFor(liveCheckFile, LiveCheck::DEFAULT_TIMEOUT_MS);
    }
    if(startFailed)
    {
        fail(QObject::tr("The new version didn't start or is too old and doesn't respond to startup checks."), true);
        return;
    }
    QDir(m_root).rmdir(m_stagingDir);
    m_succeeded = true;
    if(m_interactive)
    {
        qApp->quit();
    }
}

void InstallController::fail(const QString &message, bool ask)
{
    qWarning() << "Update failed!" << message;
    m_error = message;
    if(!m_interactive)
    {
        // nothing to ask: always back to what was there
        if(m_installer && !m_installer->rollback())
        {
            m_error += "\nThe rollback failed too.";
        }
        return;
    }
    auto failTitle = QObject::tr("Update failed!");
    bool doRollback = true;
    if(ask)
    {
        auto question = message + "\n\n" + QObject::tr("Roll back to previous version?");
        auto result = QMessageBox::critical(m_parent, failTitle, question, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        doRollback = (result == QMessageBox::Yes);
    }
    else
    {
        QMessageBox::critical(m_parent, failTitle, message);
    }
    if(!doRollback)
    {
        qApp->quit();
        return;
    }
    // nothing was changed before the Installer was made
    if(m_installer && !m_installer->rollback())
    {
        auto msg = QObject::tr("The rollback failed too.\n"
                               "You will have to repair %1 manually.\n"
                               "Please let us know why and how this happened.").arg(BuildConfig.LAUNCHER_NAME);
        QMessageBox::critical(m_parent, QObject::tr("Rollback failed!"), msg);
        qApp->quit();
    }
}

}
