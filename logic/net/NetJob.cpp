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

#include "NetJob.h"
#include "pathutils.h"
#include "MultiMC.h"
#include "MD5EtagDownload.h"
#include "ByteArrayDownload.h"
#include "CacheDownload.h"

#include "logger/QsLog.h"

void NetJob::partSucceeded(int index)
{
	// do progress. all slots are 1 in size at least
	auto &slot = parts_progress[index];
	partProgress(index, slot.total_progress, slot.total_progress);

	m_doing.remove(index);
	m_done.insert(index);
	disconnect(downloads[index].get(), 0, this, 0);
	startMoreParts();
}

void NetJob::partFailed(int index)
{
	m_doing.remove(index);
	auto &slot = parts_progress[index];
	if (slot.failures == 3)
	{
		m_failed.insert(index);
	}
	else
	{
		slot.failures++;
		m_todo.enqueue(index);
	}
	disconnect(downloads[index].get(), 0, this, 0);
	startMoreParts();
}

void NetJob::partProgress(int index, qint64 bytesReceived, qint64 bytesTotal)
{
	auto &slot = parts_progress[index];

	current_progress -= slot.current_progress;
	slot.current_progress = bytesReceived;
	current_progress += slot.current_progress;

	total_progress -= slot.total_progress;
	slot.total_progress = bytesTotal;
	total_progress += slot.total_progress;
	emit progress(current_progress, total_progress);
}

void NetJob::start()
{
	QLOG_INFO() << m_job_name.toLocal8Bit() << " started.";
	m_running = true;
	for (int i = 0; i < downloads.size(); i++)
	{
		m_todo.enqueue(i);
	}
	startMoreParts();
}

void NetJob::startMoreParts()
{
	// check for final conditions if there's nothing in the queue
	if(!m_todo.size())
	{
		if(!m_doing.size())
		{
			if(!m_failed.size())
			{
				QLOG_INFO() << m_job_name.toLocal8Bit() << "succeeded.";
				emit succeeded();
			}
			else
			{
				QLOG_ERROR() << m_job_name.toLocal8Bit() << "failed.";
				emit failed();
			}
		}
		return;
	}
	// otherwise try to start more parts
	while (m_doing.size() < 6)
	{
		if(!m_todo.size())
			return;
		int doThis = m_todo.dequeue();
		m_doing.insert(doThis);
		auto part = downloads[doThis];
		// connect signals :D
		connect(part.get(), SIGNAL(succeeded(int)), SLOT(partSucceeded(int)));
		connect(part.get(), SIGNAL(failed(int)), SLOT(partFailed(int)));
		connect(part.get(), SIGNAL(progress(int, qint64, qint64)),
				SLOT(partProgress(int, qint64, qint64)));
		part->start();
	}
}


QStringList NetJob::getFailedFiles()
{
	QStringList failed;
	for (auto index: m_failed)
	{
		failed.push_back(downloads[index]->m_url.toString());
	}
	failed.sort();
	return failed;
}
