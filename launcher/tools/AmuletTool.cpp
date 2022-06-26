#include "MCEditTool.h"

#include <QDir>
#include <QProcess>
#include <QUrl>

#include "settings/SettingsObject.h"
#include "BaseInstance.h"
#include "minecraft/MinecraftInstance.h"

MCEditTool::MCEditTool(SettingsObjectPtr settings)
{
    settings->registerSetting("AmuletPath");
    m_settings = settings;
}

void MCEditTool::setPath(QString& path)
{
    m_settings->set("AmuletPath", path);
}

QString MCEditTool::path() const
{
    return m_settings->get("AmuletPath").toString();
}

bool MCEditTool::check(const QString& toolPath, QString& error)
{
    if (toolPath.isEmpty())
    {
        error = QObject::tr("Path is empty");
        return false;
    }
    const QDir dir(toolPath);
    if (!dir.exists())
    {
        error = QObject::tr("Path does not exist");
        return false;
    }
    if (!dir.exists("amulet_app.exe"))
    {
        error = QObject::tr("Path does not seem to be an Amulet path");
        return false;
    }
    return true;
}

QString MCEditTool::getProgramPath()
{
#if defined(Q_OS_WIN32) // no amulet on anything other than windows
    if (mceditDir.exists("amulet_app.exe"))
    {
        return mceditDir.absoluteFilePath("amulet_app.exe");
    }
    return QString();
#endif
}
