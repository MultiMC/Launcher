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
#include <QHash>
#include <memory>

#include "logic/quickmod/QuickMod.h"

#include "LogicalGui.h"

class QWidget;

class OneSixInstance;
class QuickModVersion;
class QuickMod;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;

class QuickModDependencyResolver : public Bindable
{
	Q_OBJECT
public:
	explicit QuickModDependencyResolver(std::shared_ptr<OneSixInstance> instance,
										QObject *parent = 0);

	QList<QuickModVersionPtr> resolve(const QList<QuickModUid> &mods);

	QList<QuickModUid> resolveOrphans() const;
	bool hasResolveError() const;

signals:
	void error(const QString &message);
	void warning(const QString &message);
	void success(const QString &message);

private:
	std::shared_ptr<OneSixInstance> m_instance;

	QHash<QuickMod *, QuickModVersionPtr> m_mods;

	QuickModVersionPtr getVersion(const QuickModUid &modUid, const QString &filter, bool *ok);

	void resolve(const QuickModVersionPtr version);
};
