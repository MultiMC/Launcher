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

#include "QuickModRef.h"

#include "QuickModsList.h"
#include "MultiMC.h"

QuickModRef::QuickModRef() : m_uid(QString()) {}

QuickModRef::QuickModRef(const QString &uid) : m_uid(uid) {}

QuickModRef::QuickModRef(const QString &uid, const QUrl &updateUrl)
	: m_uid(uid), m_updateUrl(updateUrl)
{
}

QString QuickModRef::userFacing() const
{
	const auto mod = MMC->quickmodslist()->mod(*this);
	return mod ? mod->name() : m_uid;
}

QString QuickModRef::toString() const { return m_uid; }

QDebug operator<<(QDebug dbg, const QuickModRef &uid)
{
	return dbg << QString("QuickModRef(%1)").arg(uid.toString()).toLatin1().constData();
}

uint qHash(const QuickModRef &uid) { return qHash(uid.toString()); }
