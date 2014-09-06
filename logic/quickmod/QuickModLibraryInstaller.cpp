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

#include "QuickModLibraryInstaller.h"

#include "logic/tasks/Task.h"
#include "logic/OneSixInstance.h"

QuickModLibraryInstaller::QuickModLibraryInstaller(QuickModVersionPtr version)
	: BaseInstaller(), m_version(version)
{
}
bool QuickModLibraryInstaller::add(OneSixInstance *to)
{
	if (!BaseInstaller::add(to))
	{
		return false;
	}

	QJsonObject obj;
	obj.insert("order", qMin(to->getFullVersion()->getHighestOrder(), 99) + 1);
	obj.insert("name", m_version->mod->name());
	obj.insert("fileId", id());
	obj.insert("version", m_version->name());
	obj.insert("mcVersion", to->intendedVersionId());

	QJsonArray libraries;
	for (auto lib : m_version->libraries)
	{
		QJsonObject libObj;
		libObj.insert("name", lib.name);
		const QString urlString = lib.repo.toString(QUrl::FullyEncoded);
		libObj.insert("url", urlString);
		libObj.insert("insert", QString("prepend"));
		libObj.insert("MMC-depend", QString("soft"));
		libObj.insert("MMC-hint", QString("recurse"));
		libraries.append(libObj);
	}
	obj.insert("+libraries", libraries);

	QFile file(filename(to->instanceRoot()));
	if (!file.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Error opening" << file.fileName()
					 << "for reading:" << file.errorString();
		return false;
	}
	file.write(QJsonDocument(obj).toJson());
	file.close();

	return true;
}

ProgressProvider *QuickModLibraryInstaller::createInstallTask(OneSixInstance *instance,
															  BaseVersionPtr version,
															  QObject *parent)
{
	return 0;
}
