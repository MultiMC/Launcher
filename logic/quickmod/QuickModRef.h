/* Copyright 2014 MultiMC Contributors
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

#include <QUrl>
#include <QString>
#include <memory>

typedef std::shared_ptr<class QuickMod> QuickModPtr;
class QuickModVersionRef;

class QuickModRef
{
public:
	QuickModRef();
	explicit QuickModRef(const QString &uid);
	explicit QuickModRef(const QString &uid, const QUrl &updateUrl);

	QUrl updateUrl() const { return m_updateUrl; }
	QString userFacing() const;
	QString toString() const;
	QuickModPtr findMod() const;
	QList<QuickModPtr> findMods() const;
	QList<QuickModVersionRef> findVersions() const;

	bool isValid() const { return !m_uid.isEmpty(); }

	bool operator==(const QuickModRef &other) const { return m_uid == other.m_uid; }
	bool operator==(const QString &other) const { return m_uid == other; }
	bool operator<(const QuickModRef &other) const { return m_uid < other.m_uid; }

private:
	QString m_uid;
	QUrl m_updateUrl;
};

QDebug operator<<(QDebug dbg, const QuickModRef &uid);
uint qHash(const QuickModRef &uid);
