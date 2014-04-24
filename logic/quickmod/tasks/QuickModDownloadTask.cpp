#include "QuickModDownloadTask.h"

#include <QMessageBox>
#include <QPushButton>

#include "gui/dialogs/quickmod/QuickModInstallDialog.h"

#include "logic/net/ByteArrayDownload.h"
#include "logic/quickmod/QuickMod.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModsList.h"
#include "logic/OneSixInstance.h"
#include "MultiMC.h"

QuickModDownloadTask::QuickModDownloadTask(OneSixInstance *instance, QObject *parent)
	: Task(parent), m_instance(instance)
{

}

void QuickModDownloadTask::executeTask()
{
	const QMap<QString, QString> quickmods = m_instance->getFullVersion()->quickmods;
	auto list = MMC->quickmodslist();
	QList<QuickMod *> mods;
	for (auto it = quickmods.cbegin(); it != quickmods.cend(); ++it)
	{
		QuickMod *mod = list->mod(it.key());
		if (mod == 0)
		{
			// TODO fetch info from somewhere?
			int answer = QMessageBox::warning(0, tr("Mod not available"), tr("You seem to be missing the QuickMod file for %1. Skip it?").arg(it.key()), QMessageBox::No, QMessageBox::Yes);
			if (answer == QMessageBox::No)
			{
				emitFailed(tr("Missing %1").arg(it.key()));
				return;
			}
			else
			{
				continue;
			}
		}
		else
		{
			if (it.value().isEmpty() || !list->isModMarkedAsExists(mod, it.value()))
			{
				mods.append(mod);
			}
		}
	}

	if (mods.isEmpty())
	{
		emitSucceeded();
		return;
	}

	QuickModInstallDialog dialog(InstancePtr(m_instance), 0);
	dialog.setInitialMods(mods);
	if (dialog.exec() == QuickModInstallDialog::Accepted)
	{
		QFile f(m_instance->instanceRoot() + "/user.json");
		if (f.open(QFile::ReadWrite))
		{
			QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
			QJsonObject mods = obj.value("+mods").toObject();
			for (auto version : dialog.modVersions())
			{
				mods.insert(version->mod->uid(), version->name());
			}
			obj.insert("+mods", mods);
			f.seek(0);
			f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
			f.close();
		}
		else
		{
			QLOG_ERROR() << "Couldn't open" << f.fileName() << ". This means that stuff will be downloaded on every instance launch";
		}

		emitSucceeded();
	}
	else
	{
		emitFailed(tr("Failure downloading QuickMods"));
	}
}
