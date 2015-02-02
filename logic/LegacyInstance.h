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

#include "BaseInstance.h"
#include "gui/pages/BasePageProvider.h"

class ModList;
class Task;

class LegacyInstance : public BaseInstance, public BasePageProvider
{
	Q_OBJECT
public:

	explicit LegacyInstance(const QString &rootDir, SettingsObject *settings,
							QObject *parent = 0);

	/// Path to the instance's minecraft.jar
	QString runnableJar() const;

	//! Path to the instance's modlist file.
	QString modListFile() const;

	////// Edit Instance Dialog stuff //////
	virtual QList<BasePage *> getPages();
	virtual QString dialogTitle();

	//////  Mod Lists  //////
	std::shared_ptr<ModList> jarModList();
	std::shared_ptr<ModList> coreModList();
	std::shared_ptr<ModList> loaderModList();
	std::shared_ptr<ModList> texturePackList();

	////// Directories //////
	QString libDir() const;
	QString savesDir() const;
	QString texturePacksDir() const;
	QString jarModsDir() const;
	QString binDir() const;
	QString loaderModsDir() const;
	QString coreModsDir() const;
	QString resourceDir() const;
	virtual QString instanceConfigFolder() const override;

		/// Get the curent base jar of this instance. By default, it's the
	/// versions/$version/$version.jar
	QString baseJar() const;

	/// the default base jar of this instance
	QString defaultBaseJar() const;
	/// the default custom base jar of this instance
	QString defaultCustomBaseJar() const;

	/*!
	 * Whether or not custom base jar is used
	 */
	bool shouldUseCustomBaseJar() const;
	void setShouldUseCustomBaseJar(bool val);

	/*!
	 * The value of the custom base jar
	 */
	QString customBaseJar() const;
	void setCustomBaseJar(QString val);

	/*!
	 * Whether or not the instance's minecraft.jar needs to be rebuilt.
	 * If this is true, when the instance launches, its jar mods will be
	 * re-added to a fresh minecraft.jar file.
	 */
	bool shouldRebuild() const;
	void setShouldRebuild(bool val);

	virtual QString currentVersionId() const override;

	//! The version of LWJGL that this instance uses.
	QString lwjglVersion() const;
	/// st the version of LWJGL libs this instance will use
	void setLWJGLVersion(QString val);

	virtual QString intendedVersionId() const override;
	virtual bool setIntendedVersionId(QString version) override;
	// the `version' of Legacy instances is defined by the launcher code.
	// in contrast with OneSix, where `version' is described in a json file
	virtual bool versionIsCustom() override
	{
		return false;
	}

	virtual QSet<QString> traits()
	{
		return {"legacy-instance", "texturepacks"};
	};

	virtual bool shouldUpdate() const override;
	virtual void setShouldUpdate(bool val) override;
	virtual std::shared_ptr<Task> doUpdate() override;

	virtual bool prepareForLaunch(AuthSessionPtr account, QString & launchScript) override;
	virtual void cleanupAfterRun() override;

	virtual QString getStatusbarDescription() override;

protected:
	std::shared_ptr<ModList> jar_mod_list;
	std::shared_ptr<ModList> core_mod_list;
	std::shared_ptr<ModList> loader_mod_list;
	std::shared_ptr<ModList> texture_pack_list;

protected
slots:
	virtual void jarModsChanged();
};
