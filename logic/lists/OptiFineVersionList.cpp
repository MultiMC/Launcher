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

#include "OptiFineVersionList.h"
#include "MultiMC.h"
#include "logic/net/URLConstants.h"
#include "logic/tasks/ThreadTask.h"
#include "javautils.h"
#include "logic/OptiFineInstaller.h"

#include <QtAlgorithms>
#include <QFileDialog>

OptiFineVersionList::OptiFineVersionList(QObject *parent) : BaseVersionList(parent)
{
}

Task *OptiFineVersionList::getLoadTask()
{
	return new ThreadTask(new OptiFineListLoadTask(this), this);
}

bool OptiFineVersionList::isLoaded()
{
	return m_loaded;
}

const BaseVersionPtr OptiFineVersionList::at(int i) const
{
	return m_vlist.at(i);
}

int OptiFineVersionList::count() const
{
	return m_vlist.count();
}

void OptiFineVersionList::sort()
{
	beginResetModel();
	qSort(m_vlist);
	endResetModel();
}

void OptiFineVersionList::others(QWidget *widgetParent)
{
	const QString filename = QFileDialog::getOpenFileName(widgetParent, tr("Select OptiFine jar..."), QDir::homePath(), tr("Jars (*.jar)"));
	if (filename.isNull())
	{
		return;
	}
	const javautils::OptiFineParsedVersion ofv = javautils::getOptiFineVersionInfoFromJar(filename);
	if (!ofv.isValid())
	{
		return;
	}
	beginResetModel();
	m_vlist.append(OptiFineVersionPtr(new OptiFineVersion(QFileInfo(filename), ofv.MC_VERSION, ofv.OF_RELEASE, ofv.OF_EDITON)));
	qSort(m_vlist);
	endResetModel();
}

BaseVersionPtr OptiFineVersionList::getLatestStable() const
{
	return std::dynamic_pointer_cast<OptiFineVersion>(m_vlist.at(0));
}

void OptiFineVersionList::updateListData(QList<BaseVersionPtr> versions)
{
	beginResetModel();
	m_vlist = versions;
	m_loaded = true;
	qSort(m_vlist);
	endResetModel();
}

OptiFineListLoadTask::OptiFineListLoadTask(OptiFineVersionList *vlist)
{
	m_list = vlist;
}

OptiFineListLoadTask::~OptiFineListLoadTask()
{
}

void OptiFineListLoadTask::executeTask()
{
	setStatus(tr("Reading OptiFine versions..."));
	QList<BaseVersionPtr> out;
	OptiFineInstaller installer;
	const QStringList possibleVersions = installer.getExistingVersions();
	for (auto version : possibleVersions)
	{
		const QString filename = installer.fileForVersion(version).absoluteFilePath();
		javautils::OptiFineParsedVersion ofv = javautils::getOptiFineVersionInfoFromJar(filename);
		if (!ofv.isValid())
		{
			continue;
		}
		out += OptiFineVersionPtr(new OptiFineVersion(QFileInfo(filename), ofv.MC_VERSION, ofv.OF_RELEASE, ofv.OF_EDITON));
	}
	m_list->updateListData(out);
	emitSucceeded();
}
