/* Copyright 2014 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QWidget>
#include <memory>

class Task;
class QuickModRef;
class OneSixInstance;
class Bindable;
typedef std::shared_ptr<class QuickModVersion> QuickModVersionPtr;
typedef std::shared_ptr<struct ForgeVersion> ForgeVersionPtr;
typedef std::shared_ptr<struct LiteLoaderVersion> LiteLoaderVersionPtr;

class QuickModGuiUtil : public QWidget
{
	Q_OBJECT
	explicit QuickModGuiUtil(QWidget *parent = 0);

public:
	static void setup(std::shared_ptr<Task> task, QWidget *widgetParent);
	static void setup(Bindable *task, QWidget *widgetParent);

public
slots:
	bool modMissing(const QString &id);
	QList<QuickModVersionPtr> installMods(std::shared_ptr<OneSixInstance> instance, const QList<QuickModRef> &mods, bool *ok);
	ForgeVersionPtr getForgeVersion(std::shared_ptr<OneSixInstance> instance, const QStringList &versionFilters);
	LiteLoaderVersionPtr getLiteLoaderVersion(std::shared_ptr<OneSixInstance> instance, const QStringList &versionFilters);
};
