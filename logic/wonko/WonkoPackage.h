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

#pragma once

#include <QObject>
#include <QList>
#include <QSet>

#include "BaseVersionList.h"
#include "tasks/Task.h"
#include "net/NetJob.h"

class CachedListLoadTask;
class CachedVersionUpdateTask;

/** The versions list also represents a package (since a package is basically a list of
 * versions, with some extra metadata attached)
 */
class WonkoPackage : public BaseVersionList,
						 public std::enable_shared_from_this<WonkoPackage>
{
	Q_OBJECT
public:
	enum LoadStatus
	{
		NotLoaded,   //< The list has not been loaded yet
		LocalLoaded, //< The list has been loaded using local (cached) data
		RemoteLoaded //< The list has been loaded from a remote source
	};

private:
	friend class CachedListLoadTask;
	void loadListFromJSON(const QJsonDocument &jsonDoc, const LoadStatus source);
	void sortInternal(); //< Helper
	void reindex();		 //< Rebuilds the m_lookup table

public:
	explicit WonkoPackage(const QString &baseUrl, const QString &uid,
							  QObject *parent = nullptr);

	void loadFromIndex(const QJsonObject &obj, const LoadStatus source);

	std::shared_ptr<Task> createUpdateTask(const QString &version);

	Task *getLoadTask() override;
	bool isLoaded() override;
	const BaseVersionPtr at(int i) const override;
	int count() const override;
	void sort() override;
	virtual QVariant data(const QModelIndex & index, int role) const override;
	virtual RoleList providesRoles() override;

	BaseVersionPtr getLatestStable() const override;

	QString uid() const
	{
		return m_uid;
	}
	QString indexUrl() const
	{
		return m_baseUrl + QString("%1.json").arg(m_uid);
	}
	QString fileUrl(QString id) const
	{
		return m_baseUrl + QString("%1/%2.json").arg(m_uid).arg(id);
	}
	QString versionFilePath(QString version) const;

private slots:
	void initialLoad();

protected:
	QList<BaseVersionPtr> m_vlist;
	QMap<QString, BaseVersionPtr> m_lookup;

	LoadStatus m_loaded = NotLoaded;
	bool m_isFull = false;

	QString m_latestReleaseID = "INVALID";
	QString m_latestSnapshotID = "INVALID";
	QString m_baseUrl;
	QString m_uid;

protected slots:
	virtual void updateListData(QList<BaseVersionPtr> versions);
};
