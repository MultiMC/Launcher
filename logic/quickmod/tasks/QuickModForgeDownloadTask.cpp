#include "QuickModForgeDownloadTask.h"

#include "logic/ForgeInstaller.h"
#include "logic/lists/ForgeVersionList.h"
#include "logic/OneSixInstance.h"
#include "logic/quickmod/QuickModsList.h"
#include "logic/quickmod/QuickMod.h"
#include "MultiMC.h"

#include "gui/dialogs/VersionSelectDialog.h"

QuickModForgeDownloadTask::QuickModForgeDownloadTask(InstancePtr instance, QObject *parent)
	: Task(parent), m_instance(instance)
{

}

void QuickModForgeDownloadTask::executeTask()
{
	auto mods = std::dynamic_pointer_cast<OneSixInstance>(m_instance)->getFullVersion()->quickmods;
	if (mods.isEmpty())
	{
		emitSucceeded();
		return;
	}

	ForgeInstaller *installer = new ForgeInstaller;
	if (QDir(m_instance->instanceRoot()).exists("patches/" + installer->id() + ".json"))
	{
		emitSucceeded();
		return;
	}

	QStringList versionFilters;
	for (auto it = mods.cbegin(); it != mods.cend(); ++it)
	{
		QuickModPtr mod = MMC->quickmodslist()->mod(it.key());
		if (!mod)
		{
			continue;
		}
		QuickModVersionPtr version = mod->version(it.value());
		if (!version)
		{
			continue;
		}
		if (!version->forgeVersionFilter.isEmpty())
		{
			versionFilters.append(version->forgeVersionFilter);
		}
	}

	VersionSelectDialog vselect(MMC->forgelist().get(), tr("Select Forge version"));
	if (versionFilters.isEmpty())
	{
		vselect.setFilter(1, m_instance->currentVersionId());
	}
	else
	{
		vselect.setFilter(0, versionFilters.join(','));
	}
	vselect.setEmptyString(tr("No Forge versions are currently available for Minecraft ") +
						   m_instance->currentVersionId());
	if (vselect.exec() && vselect.selectedVersion())
	{
		auto task = installer->createInstallTask(std::dynamic_pointer_cast<OneSixInstance>(m_instance).get(), vselect.selectedVersion(), this);
		connect(task, &Task::progress, [this](qint64 current, qint64 total){setProgress(100 * current / qMax((qint64)1, total));});
		connect(task, &Task::status, [this](const QString &msg){setStatus(msg);});
		connect(task, &Task::failed, [this,installer](const QString &reason){delete installer;qDebug("failed");emitFailed(reason);});
		connect(task, &Task::succeeded, [this,installer](){delete installer;qDebug("succeeded");emitSucceeded();});
		task->start();
	}
	else
	{
		emitFailed(tr("No forge version selected"));
	}
}
