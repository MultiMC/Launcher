/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "QuickModDownloadTask.h"

#include "logic/net/ByteArrayDownload.h"
#include "logic/quickmod/QuickMod.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModsList.h"
#include "logic/OneSixInstance.h"
#include "MultiMC.h"

// FIXME this entire thing needs work. possible remove it?

QuickModDownloadTask::QuickModDownloadTask(std::shared_ptr<OneSixInstance> instance,
										   QObject *parent)
	: Task(parent), m_instance(instance)
{
}

void QuickModDownloadTask::executeTask()
{
	const QMap<QuickModUid, QString> quickmods =
		std::dynamic_pointer_cast<OneSixInstance>(m_instance)->getFullVersion()->quickmods;
	auto list = MMC->quickmodslist();
	QList<QuickModUid> mods;
	qDebug() << quickmods;
	for (auto it = quickmods.cbegin(); it != quickmods.cend(); ++it)
	{
		qDebug() << it.value() << it.key();
		QuickModPtr mod = it.value().isEmpty() ? list->mods(it.key()).first()
											   : list->modVersion(it.key(), it.value())->mod;
		if (mod == 0)
		{
			// TODO fetch info from somewhere?
			if (wait<bool>("QuickMods.ModMissing", it.key().toString()))
			{
				emitFailed(tr("Missing %1").arg(it.key().toString()));
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
				mods.append(mod->uid());
			}
		}
	}

	qDebug() << mods;
	if (mods.isEmpty())
	{
		emitSucceeded();
		return;
	}

	bool ok = false;
	QList<QuickModVersionPtr> installedVersions =
		wait<QList<QuickModVersionPtr>>("QuickMods.InstallMods", m_instance, mods, &ok);
	if (ok)
	{
		QFile f(m_instance->instanceRoot() + "/user.json");
		if (f.open(QFile::ReadWrite))
		{
			QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
			QJsonObject mods = obj.value("+mods").toObject();
			for (auto version : installedVersions)
			{
				mods.insert(version->mod->uid().toString(), version->name());
			}
			obj.insert("+mods", mods);
			f.seek(0);
			f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
			f.close();
		}
		else
		{
			QLOG_ERROR()
				<< "Couldn't open" << f.fileName()
				<< ". This means that stuff will be downloaded on every instance launch";
		}

		emitSucceeded();
	}
	else
	{
		emitFailed(tr("Failure downloading QuickMods"));
	}
}
