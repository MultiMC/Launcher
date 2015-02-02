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
#include <QDateTime>
#include <QSet>

#include "logic/settings/SettingsObject.h"

#include "logic/settings/INIFile.h"
#include "logic/BaseVersionList.h"
#include "logic/auth/MojangAccount.h"

class ModList;
class QDialog;
class QDir;
class Task;
class MinecraftProcess;
class OneSixUpdate;
class InstanceList;
class BaseInstancePrivate;

// pointer for lazy people
class BaseInstance;
typedef std::shared_ptr<BaseInstance> InstancePtr;

/*!
 * \brief Base class for instances.
 * This class implements many functions that are common between instances and
 * provides a standard interface for all instances.
 *
 * To create a new instance type, create a new class inheriting from this class
 * and implement the pure virtual functions.
 */
class BaseInstance : public QObject
{
	Q_OBJECT
protected:
	/// no-touchy!
	BaseInstance(const QString &rootDir, SettingsObject *settings, QObject *parent = 0);

public:
	/// virtual destructor to make sure the destruction is COMPLETE
	virtual ~BaseInstance() {};

	virtual void init() {}
	virtual void copy(const QDir &newDir) {}

	/// nuke thoroughly - deletes the instance contents, notifies the list/model which is
	/// responsible of cleaning up the husk
	void nuke();

	/// The instance's ID. The ID SHALL be determined by MMC internally. The ID IS guaranteed to
	/// be unique.
	virtual QString id() const;

	void setRunning(bool running);
	bool isRunning() const;

	/// get the type of this instance
	QString instanceType() const;

	/// Path to the instance's root directory.
	QString instanceRoot() const;

	/// Path to the instance's minecraft directory.
	QString minecraftRoot() const;

	QString name() const;
	void setName(QString val);

	/// Value used for instance window titles
	QString windowTitle() const;

	QString iconKey() const;
	void setIconKey(QString val);

	QString notes() const;
	void setNotes(QString val);

	QString group() const;
	void setGroupInitial(QString val);
	void setGroupPost(QString val);

	virtual QStringList extraArguments() const;

	virtual QString intendedVersionId() const = 0;
	virtual bool setIntendedVersionId(QString version) = 0;

	virtual bool versionIsCustom() = 0;

	/*!
	 * The instance's current version.
	 * This value represents the instance's current version. If this value is
	 * different from the intendedVersion, the instance should be updated.
	 * \warning Don't change this value unless you know what you're doing.
	 */
	virtual QString currentVersionId() const = 0;

	/*!
	 * Whether or not Minecraft should be downloaded when the instance is launched.
	 */
	virtual bool shouldUpdate() const = 0;
	virtual void setShouldUpdate(bool val) = 0;

	//////  Mod Lists  //////
	virtual std::shared_ptr<ModList> resourcePackList()
	{
		return nullptr;
	}
	virtual std::shared_ptr<ModList> texturePackList()
	{
		return nullptr;
	}

	/// Traits. Normally inside the version, depends on instance implementation.
	virtual QSet <QString> traits() = 0;

	/**
	 * Gets the time that the instance was last launched.
	 * Stored in milliseconds since epoch.
	 */
	qint64 lastLaunch() const;
	/// Sets the last launched time to 'val' milliseconds since epoch
	void setLastLaunch(qint64 val = QDateTime::currentMSecsSinceEpoch());

	/*!
	 * \brief Gets the instance list that this instance is a part of.
	 *        Returns NULL if this instance is not in a list
	 *        (the parent is not an InstanceList).
	 * \return A pointer to the InstanceList containing this instance.
	 */
	InstanceList *instList() const;

	InstancePtr getSharedPtr();

	/*!
	 * \brief Gets a pointer to this instance's version list.
	 * \return A pointer to the available version list for this instance.
	 */
	virtual std::shared_ptr<BaseVersionList> versionList() const;

	/*!
	 * \brief Gets this instance's settings object.
	 * This settings object stores instance-specific settings.
	 * \return A pointer to this instance's settings object.
	 */
	virtual SettingsObject &settings() const;

	/// returns a valid update task
	virtual std::shared_ptr<Task> doUpdate() = 0;

	/// returns a valid minecraft process, ready for launch with the given account.
	virtual bool prepareForLaunch(AuthSessionPtr account, QString & launchScript) = 0;

	/// do any necessary cleanups after the instance finishes. also runs before
	/// 'prepareForLaunch'
	virtual void cleanupAfterRun() = 0;

	virtual QString getStatusbarDescription() = 0;

	/// FIXME: this really should be elsewhere...
	virtual QString instanceConfigFolder() const = 0;

	enum InstanceFlag
	{
		VersionBrokenFlag = 0x01,
		UpdateAvailable = 0x02
	};
	Q_DECLARE_FLAGS(InstanceFlags, InstanceFlag);
	InstanceFlags flags() const;
	void setFlags(const InstanceFlags &flags);
	void setFlag(const InstanceFlag flag);
	void unsetFlag(const InstanceFlag flag);

	bool canLaunch() const;

	virtual bool reload();

signals:
	/*!
	 * \brief Signal emitted when properties relevant to the instance view change
	 */
	void propertiesChanged(BaseInstance *inst);
	/*!
	 * \brief Signal emitted when groups are affected in any way
	 */
	void groupChanged();
	/*!
	 * \brief The instance just got nuked. Hurray!
	 */
	void nuked(BaseInstance *inst);

	void flagsChanged();

protected slots:
	void iconUpdated(QString key);

protected:
	QString m_rootDir;
	QString m_group;
	std::shared_ptr<SettingsObject> m_settings;
	InstanceFlags m_flags;
	bool m_isRunning = false;
};

Q_DECLARE_METATYPE(std::shared_ptr<BaseInstance>)
Q_DECLARE_METATYPE(BaseInstance::InstanceFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(BaseInstance::InstanceFlags)
