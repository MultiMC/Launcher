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

#include <QStringList>
#include <QSet>
#include <QDateTime>

#include "logic/BaseVersion.h"
#include "VersionPatch.h"
#include "VersionFile.h"
#include "VersionSource.h"

class InstanceVersion;
class MinecraftVersion;
typedef std::shared_ptr<MinecraftVersion> MinecraftVersionPtr;

class MinecraftVersion : public BaseVersion, public VersionPatch
{
public: /* methods */
	bool usesLegacyLauncher();
	virtual QString descriptor() override;
	virtual QString name() override;
	virtual QString typeString() const override;
	virtual bool hasJarMods() override;
	virtual bool isMinecraftVersion() override;
	virtual void applyTo(InstanceVersion *version) override;
	virtual int getOrder();
	virtual void setOrder(int order);
	virtual QList<JarmodPtr> getJarMods() override;
	virtual QString getPatchID() override;
	virtual QString getPatchVersion() override;
	virtual QString getPatchName() override;
	virtual QString getPatchFilename() override;
	bool needsUpdate();
	bool hasUpdate();
	virtual bool isCustom();

private: /* methods */
	void applyFileTo(InstanceVersion *version);

public: /* data */
	/// The URL that this version will be downloaded from. maybe.
	QString download_url;

	VersionSource m_versionSource = Builtin;

	/// the human readable version name
	QString m_name;

	/// the version ID.
	QString m_descriptor;

	/// version traits. added by MultiMC
	QSet<QString> m_traits;

	/// The main class this version uses (if any, can be empty).
	QString m_mainClass;

	/// The applet class this version uses (if any, can be empty).
	QString m_appletClass;

	/// The process arguments used by this version
	QString m_processArguments;

	/// The type of this release
	QString m_type;

	/// the time this version was actually released by Mojang, as string and as QDateTime
	QString m_releaseTimeString;
	QDateTime m_releaseTime;

	/// the time this version was last updated by Mojang, as string and as QDateTime
	QString m_updateTimeString;
	QDateTime m_updateTime;

	/// MD5 hash of the minecraft jar
	QString m_jarChecksum;

	/// order of this file... default = -2
	int order = -2;
	
	/// an update available from Mojang
	MinecraftVersionPtr upstreamUpdate;
};
