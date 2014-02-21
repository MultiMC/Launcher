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
#include <QString>
#include <settingsobject.h>

class BaseInstance;

#define I_D(Class) Class##Private *const d = (Class##Private * const)inst_d.get()

struct BaseInstancePrivate
{
	QString m_rootDir;
	QString m_group;
	SettingsObject *m_settings;
	int m_flags;
};
