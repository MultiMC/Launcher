/* Copyright 2013-2015 MultiMC Contributors
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

#include "Env.h"
#include "OneSixUpdate.h"

#include <QtNetwork>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDataStream>
#include <pathutils.h>
#include <JlCompress.h>

#include "BaseInstance.h"
#include "wonko/WonkoPackage.h"
#include "minecraft/MinecraftProfile.h"
#include "minecraft/Library.h"
#include "minecraft/onesix/OneSixInstance.h"
#include "net/URLConstants.h"
#include "MMCZip.h"
#include <tasks/SequentialTask.h>

OneSixUpdate::OneSixUpdate(OneSixInstance *inst, QObject *parent) : Task(parent), m_inst(inst)
{
}

void OneSixUpdate::executeTask()
{
	// Make directories
	QDir mcDir(m_inst->minecraftRoot());
	if (!mcDir.exists() && !mcDir.mkpath("."))
	{
		emitFailed(tr("Failed to create folder for minecraft binaries."));
		return;
	}

	bool updateTask = false;
	versionUpdateTask = std::make_shared<SequentialTask>();

	auto addTasklet = [&](const QString & uid, const QString & version)
	{
		if(!version.isEmpty())
		{
			auto list = std::dynamic_pointer_cast<WonkoPackage>(ENV.getVersionList(uid));
			versionUpdateTask->addTask(std::shared_ptr<Task>(list->getLoadTask()));
			versionUpdateTask->addTask(list->createUpdateTask(version));
			updateTask = true;
		}
	};

	addTasklet("net.minecraft", m_inst->minecraftVersion());
	addTasklet("org.lwjgl", m_inst->lwjglVersion());
	addTasklet("com.mumfrey.liteloader", m_inst->liteloaderVersion());
	addTasklet("net.minecraftforge", m_inst->forgeVersion());

	if (!updateTask)
	{
		qDebug() << "Didn't spawn an update task.";
		emitSucceeded();
		return;
	}

	connect(versionUpdateTask.get(), SIGNAL(succeeded()), SLOT(versionUpdateSucceeded()));
	connect(versionUpdateTask.get(), &NetJob::failed, this, &OneSixUpdate::versionUpdateFailed);
	connect(versionUpdateTask.get(), SIGNAL(progress(qint64, qint64)),SIGNAL(progress(qint64, qint64)));
	setStatus(tr("Getting the version files from Mojang..."));
	versionUpdateTask->start();
}

void OneSixUpdate::versionUpdateSucceeded()
{
	if(m_inst->reload())
	{
		if(auto task = m_inst->getMinecraftProfile()->resources.updateTask())
		{
			connect(task, SIGNAL(succeeded()), SLOT(emitSucceeded()));
			connect(task, SIGNAL(failed(QString)), SLOT(emitFailed(QString)));
			connect(task, SIGNAL(progress(qint64,qint64)), SIGNAL(progress(qint64,qint64)));
			filesTask.reset(task);
			task->start();
			return;
		}
		emitSucceeded();
	}
	else
	{
		emitFailed(tr("Failed to load instance profile"));
	}
}

void OneSixUpdate::versionUpdateFailed(QString reason)
{
	emitFailed(reason);
}
