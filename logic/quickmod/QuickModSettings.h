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

#include <memory>

class QString;
template <typename Key, typename Value> class QMap;
typedef std::shared_ptr<class QuickMod> QuickModPtr;
class QuickModVersionRef;
class QuickModRef;
class BaseInstance;
class SettingsObject;
typedef std::shared_ptr<class BaseInstance> InstancePtr;

class QuickModSettings
{
public:
	void markModAsExists(QuickModPtr mod, const QuickModVersionRef &version,
						 const QString &fileName);
	void markModAsInstalled(const QuickModRef uid, const QuickModVersionRef &version,
							const QString &fileName, InstancePtr instance);
	void markModAsUninstalled(const QuickModRef uid, const QuickModVersionRef &version,
							  InstancePtr instance);
	bool isModMarkedAsInstalled(const QuickModRef uid, const QuickModVersionRef &version,
								InstancePtr instance) const;
	bool isModMarkedAsExists(QuickModPtr mod, const QuickModVersionRef &version) const;
	QMap<QuickModVersionRef, QString> installedModFiles(const QuickModRef uid,
														BaseInstance *instance) const;
	QString existingModFile(QuickModPtr mod, const QuickModVersionRef &version) const;

protected:
	explicit QuickModSettings();

	virtual SettingsObject *settings() const = 0;
};
