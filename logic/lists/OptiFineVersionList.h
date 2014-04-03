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

#pragma once

#include <QObject>

#include <QString>
#include <QStringList>
#include "BaseVersionList.h"
#include "logic/tasks/Task.h"
#include "logic/BaseVersion.h"
#include <logic/net/NetJob.h>

class OptiFineListLoadTask;
class QNetworkReply;

class OptiFineVersion : public BaseVersion
{
public:
	explicit OptiFineVersion(const QFileInfo &file, const QString &mc, const QString &release,
							 const QString &edition)
		: m_file(file), m_mcVersion(mc), m_release(release), m_edition(edition)
	{
	}

	QFileInfo file() const
	{
		return m_file;
	}

	inline QString fullVersionString() const
	{
		return m_mcVersion + "_" + versionString();
	}
	inline QString versionString() const
	{
		return m_edition + "_" + m_release;
	}

	QString descriptor() override
	{
		return m_mcVersion;
	}
	QString name() override
	{
		return m_release;
	}
	QString typeString() const override
	{
		return m_edition;
	}

private:
	QFileInfo m_file;
	QString m_mcVersion;
	QString m_release;
	QString m_edition;
};
typedef std::shared_ptr<OptiFineVersion> OptiFineVersionPtr;

class OptiFineVersionList : public BaseVersionList
{
	Q_OBJECT
public:
	friend class OptiFineListLoadTask;

	explicit OptiFineVersionList(QObject *parent = 0);

	virtual Task *getLoadTask() override;
	virtual bool isLoaded() override;
	virtual const BaseVersionPtr at(int i) const override;
	virtual int count() const override;
	virtual void sort() override;
	virtual void others(QWidget *widgetParent) override;

	virtual BaseVersionPtr getLatestStable() const;

protected:
	QList<BaseVersionPtr> m_vlist;

	bool m_loaded = false;

protected
slots:
	virtual void updateListData(QList<BaseVersionPtr> versions);
};

class OptiFineListLoadTask : public Task
{
	Q_OBJECT

public:
	explicit OptiFineListLoadTask(OptiFineVersionList *vlist);
	~OptiFineListLoadTask();

	virtual void executeTask();

protected
slots:

protected:
	NetJobPtr listJob;
	CacheDownloadPtr listDownload;
	OptiFineVersionList *m_list;
};
