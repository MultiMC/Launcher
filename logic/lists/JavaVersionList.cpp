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

#include "JavaVersionList.h"
#include "MultiMC.h"

#include <QtNetwork>
#include <QtXml>
#include <QRegExp>

#include "logger/QsLog.h"
#include "logic/JavaCheckerJob.h"
#include "logic/JavaUtils.h"

JavaVersionList::JavaVersionList(QObject *parent) : BaseVersionList(parent)
{
}

Task *JavaVersionList::getLoadTask()
{
	return new JavaListLoadTask(this);
}

const BaseVersionPtr JavaVersionList::at(int i) const
{
	return m_vlist.at(i);
}

bool JavaVersionList::isLoaded()
{
	return m_loaded;
}

int JavaVersionList::count() const
{
	return m_vlist.count();
}

int JavaVersionList::columnCount(const QModelIndex &parent) const
{
	return 3;
}

QVariant JavaVersionList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() > count())
		return QVariant();

	auto version = std::dynamic_pointer_cast<JavaVersion>(m_vlist[index.row()]);
	switch (role)
	{
	case Qt::DisplayRole:
		switch (index.column())
		{
		case 0:
			return version->id;

		case 1:
			return version->arch;

		case 2:
			return version->path;

		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
		return version->descriptor();

	case VersionPointerRole:
		return qVariantFromValue(m_vlist[index.row()]);

	default:
		return QVariant();
	}
}

QVariant JavaVersionList::headerData(int section, Qt::Orientation orientation, int role) const
{
	switch (role)
	{
	case Qt::DisplayRole:
		switch (section)
		{
		case 0:
			return "Version";

		case 1:
			return "Arch";

		case 2:
			return "Path";

		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
		switch (section)
		{
		case 0:
			return "The name of the version.";

		case 1:
			return "The architecture this version is for.";

		case 2:
			return "Path to this Java version.";

		default:
			return QVariant();
		}

	default:
		return QVariant();
	}
}

BaseVersionPtr JavaVersionList::getTopRecommended() const
{
	auto first = m_vlist.first();
	if(first != nullptr)
	{
		return first;
	}
	else
	{
		return BaseVersionPtr();
	}
}

void JavaVersionList::updateListData(QList<BaseVersionPtr> versions)
{
	beginResetModel();
	m_vlist = versions;
	m_loaded = true;
	endResetModel();
	// NOW SORT!!
	// sort();
}

void JavaVersionList::sort()
{
	// NO-OP for now
}

JavaListLoadTask::JavaListLoadTask(JavaVersionList *vlist) : Task()
{
	m_list = vlist;
	m_currentRecommended = NULL;
}

JavaListLoadTask::~JavaListLoadTask()
{
}

void JavaListLoadTask::executeTask()
{
	setStatus(tr("Detecting Java installations..."));

	JavaUtils ju;
	QList<QString> candidate_paths = ju.FindJavaPaths();

	m_job = std::shared_ptr<JavaCheckerJob>(new JavaCheckerJob("Java detection"));
	connect(m_job.get(), SIGNAL(finished(QList<JavaCheckResult>)), this, SLOT(javaCheckerFinished(QList<JavaCheckResult>)));
	connect(m_job.get(), SIGNAL(progress(int, int)), this, SLOT(checkerProgress(int, int)));

	QLOG_DEBUG() << "Probing the following Java paths: ";
	int id = 0;
	for(QString candidate : candidate_paths)
	{
		QLOG_DEBUG() << " " << candidate;

		auto candidate_checker = new JavaChecker();
		candidate_checker->path = candidate;
		candidate_checker->id = id;
		m_job->addJavaCheckerAction(JavaCheckerPtr(candidate_checker));

		id++;
	}

	m_job->start();
}

void JavaListLoadTask::checkerProgress(int current, int total)
{
	float progress = (current * 100.0) / total;
	this->setProgress((int) progress);
}

void JavaListLoadTask::javaCheckerFinished(QList<JavaCheckResult> results)
{
	QList<JavaVersionPtr> candidates;

	QLOG_DEBUG() << "Found the following valid Java installations:";
	for(JavaCheckResult result : results)
	{
		if(result.valid)
		{
			JavaVersionPtr javaVersion(new JavaVersion());

			javaVersion->id = result.javaVersion;
			javaVersion->arch = result.mojangPlatform;
			javaVersion->path = result.path;
			candidates.append(javaVersion);

			QLOG_DEBUG() << " " << javaVersion->id << javaVersion->arch << javaVersion->path;
		}
	}

	QList<BaseVersionPtr> javas_bvp;
	for (auto java : candidates)
	{
		//QLOG_INFO() << java->id << java->arch << " at " << java->path;
		BaseVersionPtr bp_java = std::dynamic_pointer_cast<BaseVersion>(java);

		if (bp_java)
		{
			javas_bvp.append(java);
		}
	}

	m_list->updateListData(javas_bvp);
	emitSucceeded();
}
