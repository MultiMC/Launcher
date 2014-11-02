#include "AMIDST.h"

#include <QDir>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QJsonDocument>
#include <QFile>
#include <QTemporaryFile>

#include "logic/settings/SettingsObject.h"
#include "logic/net/HttpMetaCache.h"
#include "logic/BaseInstance.h"
#include "logic/OneSixInstance.h"
#include "logic/MMCJson.h"
#include "MultiMC.h"
#include "pathutils.h"

AMIDSTTool::AMIDSTTool(InstancePtr instance, QObject *parent)
	: BaseDetachedTool(instance, parent)
{
}

void AMIDSTTool::runImpl()
{
	auto instance = std::dynamic_pointer_cast<OneSixInstance>(m_instance);
	if (!instance)
	{
		throw MMCError(tr("AMIDST only works with OneSix instances"));
	}
	const QString amidstPath = MMC->settings()->get("AMIDSTPath").toString();
	const QString versionId = instance->currentVersionId();
	const QString versionDir = PathCombine(MMC->metacache()->getBasePath("versions"), versionId);
	const QString tmpJsonPath = PathCombine(QDir::tempPath(), "multimc-" + versionId + ".json");
	if (QDir().exists(tmpJsonPath))
	{
		QDir().remove(tmpJsonPath);
	}
	MMCJson::writeFile(tmpJsonPath, QJsonDocument(instance->getFullVersion()->toJson()));
	QProcess::startDetached(instance->settings().get("JavaPath").toString(),
							QStringList() << "-jar" << amidstPath
							<< "-mcpath" << PathCombine(QDir::currentPath(), instance->minecraftRoot())
							<< "-mclibs" << MMC->metacache()->getBasePath("libraries")
							<< "-mcjar" << PathCombine(versionDir, versionId + ".jar")
							<< "-mcjson" << tmpJsonPath,
							instance->instanceRoot());
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
