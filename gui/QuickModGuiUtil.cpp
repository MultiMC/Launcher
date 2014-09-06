#include "QuickModGuiUtil.h"

#include <QMessageBox>

#include "gui/dialogs/VersionSelectDialog.h"
#include "gui/dialogs/quickmod/QuickModInstallDialog.h"
#include "logic/tasks/SequentialTask.h"
#include "logic/forge/ForgeVersionList.h"
#include "logic/liteloader/LiteLoaderVersionList.h"
#include "logic/OneSixInstance.h"
#include "LogicalGui.h"
#include "MultiMC.h"

QuickModGuiUtil::QuickModGuiUtil(QWidget *parent) : QWidget(parent)
{
	hide();
}

void QuickModGuiUtil::setup(Bindable *task, QWidget *widgetParent)
{
	QuickModGuiUtil *util = new QuickModGuiUtil(widgetParent);
	task->bind("QuickMods.ModMissing", util, &QuickModGuiUtil::modMissing);
	task->bind("QuickMods.InstallMods", util, &QuickModGuiUtil::installMods);
	task->bind("QuickMods.GetForgeVersion", util, &QuickModGuiUtil::getForgeVersion);
	task->bind("QuickMods.GetLiteLoaderVersion", util, &QuickModGuiUtil::getLiteLoaderVersion);
}

bool QuickModGuiUtil::modMissing(const QString &id)
{
	return QMessageBox::warning(
			   this, tr("Mod not available"),
			   tr("You seem to be missing the QuickMod file for %1. Skip it?").arg(id),
			   QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes;
}

QList<QuickModVersionPtr> QuickModGuiUtil::installMods(std::shared_ptr<OneSixInstance> instance,
													   const QList<QuickModRef> &mods, bool *ok)
{
	QuickModInstallDialog dialog(instance, this);
	dialog.setInitialMods(mods);
	*ok = dialog.exec() == QDialog::Accepted;
	return dialog.modVersions();
}

ForgeVersionPtr QuickModGuiUtil::getForgeVersion(std::shared_ptr<OneSixInstance> instance,
												 const QStringList &versionFilters)
{
	VersionSelectDialog vselect(MMC->forgelist().get(), tr("Select Forge version"));
	if (versionFilters.isEmpty())
	{
		vselect.setExactFilter(1, instance->currentVersionId());
	}
	else
	{
		vselect.setExactFilter(0, versionFilters.join(','));
	}
	vselect.setEmptyString(tr("No Forge versions are currently available for Minecraft ") +
						   instance->currentVersionId());
	return vselect.exec() == QDialog::Accepted
			   ? std::dynamic_pointer_cast<ForgeVersion>(vselect.selectedVersion())
			   : ForgeVersionPtr();
}
LiteLoaderVersionPtr
QuickModGuiUtil::getLiteLoaderVersion(std::shared_ptr<OneSixInstance> instance,
									  const QStringList &versionFilters)
{
	VersionSelectDialog vselect(MMC->liteloaderlist().get(), tr("Select LiteLoader version"));
	if (versionFilters.isEmpty())
	{
		vselect.setExactFilter(1, instance->currentVersionId());
	}
	else
	{
		vselect.setExactFilter(0, versionFilters.join(','));
	}
	vselect.setEmptyString(tr("No LiteLoader versions are currently available for Minecraft ") +
						   instance->currentVersionId());
	return vselect.exec() == QDialog::Accepted
			   ? std::dynamic_pointer_cast<LiteLoaderVersion>(vselect.selectedVersion())
			   : LiteLoaderVersionPtr();
}
