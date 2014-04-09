#include "AMIDST.h"

#include <QDir>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>

#include "logic/settings/SettingsObject.h"
#include "logic/BaseInstance.h"
#include "MultiMC.h"
#include "pathutils.h"

AMIDSTTool::AMIDSTTool(InstancePtr instance, QObject *parent)
	: BaseDetachedTool(instance, parent)
{
}

void AMIDSTTool::runImpl()
{
	const QString amidstPath = MMC->settings()->get("AMIDSTPath").toString();
	QProcess::startDetached(m_instance->settings().get("JavaPath").toString(),
							QStringList() << "-jar" << amidstPath
							<< "-mcpath" << PathCombine(QDir::currentPath(), m_instance->minecraftRoot())
							<< "-mcjar" << PathCombine(QDir::current().absoluteFilePath("versions"), m_instance->currentVersionId(), m_instance->currentVersionId() + ".jar")
							<< "-mcjson" << PathCombine(QDir::currentPath(), m_instance->instanceRoot(), "version.json"),
							m_instance->instanceRoot());
}

void AMIDSTFactory::registerSettings(std::shared_ptr<SettingsObject> settings)
{
	settings->registerSetting("AMIDSTPath");
}
BaseExternalTool *AMIDSTFactory::createTool(InstancePtr instance, QObject *parent)
{
	return new AMIDSTTool(instance, parent);
}
bool AMIDSTFactory::check(QString *error)
{
	return check(MMC->settings()->get("AMIDSTPath").toString(), error);
}
bool AMIDSTFactory::check(const QString &path, QString *error)
{
	if (path.isEmpty())
	{
		*error = QObject::tr("Path is empty");
		return false;
	}
	const QFileInfo file(path);
	if (!file.exists())
	{
		*error = QObject::tr("File does not exist");
		return false;
	}
	if (file.suffix() == "jar")
	{
		return true;
	}
	*error = QObject::tr("File does not seem to be AMIDST file");
	return false;
}
