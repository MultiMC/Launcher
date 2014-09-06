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
#include <QDir>
#include <memory>

#include "logic/quickmod/QuickMod.h"

class QuickModsList;
class QuickModIndexList;
class Mod;

/**
 * Takes care of regulary checking for updates to quickmod files, and is also responsible for
 * keeping track of them
 */
class QuickModFilesUpdater : public QObject
{
	Q_OBJECT
public:
	QuickModFilesUpdater(QuickModsList *list);

	void registerFile(const QUrl &url, bool sandbox);
	void unregisterMod(const QuickModPtr mod);

	void releaseFromSandbox(QuickModPtr mod);
	void cleanupSandboxedFiles();

public
slots:
	void update();

signals:
	void error(const QString &message);
	void addedSandboxedMod(QuickModPtr mod);

private
slots:
	void receivedMod(int notused);
	void failedMod(int index);
	void readModFiles();

private:
	QuickModsList *m_list;
	QDir m_quickmodDir;
	QuickModIndexList *m_indexList;

	bool parseQuickMod(const QString &fileName, QuickModPtr mod);

	static QString fileName(const QuickModPtr mod);
	static QString fileName(const QString &uid);
};
