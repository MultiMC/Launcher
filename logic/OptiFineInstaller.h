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

#include "BaseInstaller.h"

#include <QString>
#include <QMap>
#include <QFileInfo>

#include "lists/OptiFineVersionList.h"
#include "tasks/Task.h"

class OptiFineInstaller : public BaseInstaller
{
public:
	OptiFineInstaller();

	QFileInfo fileForVersion(const QString &version) const;
	QStringList getExistingVersions() const;

	void prepare(OptiFineVersionPtr version);
	bool add(OneSixInstance *to) override;
	bool canApply(const OneSixInstance *to) override;

	ProgressProvider *createInstallTask(OneSixInstance *instance, BaseVersionPtr version, QObject *parent) override;

private:
	virtual QString id() const override
	{
		return "optifine";
	}
	OptiFineVersionPtr m_version;
	static QMap<QString, QString> m_mcVersionLaunchWrapperVersionMapping;
};
