#include "MCEditTool.h"

#include <QDir>
#include <QProcess>
#include <QUrl>

#include "settings/SettingsObject.h"
#include "BaseInstance.h"
#include "minecraft/MinecraftInstance.h"

MCEditTool::MCEditTool(SettingsObjectPtr settings)
{
    settings->registerSetting("MCEditPath");
    m_settings = settings;
}

void MCEditTool::setPath(QString& path)
{
    m_settings->set("MCEditPath", path);
}

QString MCEditTool::path() const
{
    return m_settings->get("MCEditPath").toString();
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
    if (!dir.exists("mcedit.sh") && !dir.exists("mcedit.py") && !dir.exists("mcedit.exe") && !dir.exists("Contents") && !dir.exists("mcedit2.exe"))
    {
        error = QObject::tr("Path does not seem to be a MCEdit path");
        return false;
    }
    return true;
}

QString MCEditTool::getProgramPath()
{
    #ifdef Q_OS_WIN
        return "some/path";
    #elif defined(Q_OS_MAC)
        return "some/other/path";
    #elif defined(Q_OS_LINUX)
        return "yet/another/path";
    #endif
    // Добавляем значение по умолчанию для неподдерживаемых платформ, таких как Haiku
    return QString();
}
