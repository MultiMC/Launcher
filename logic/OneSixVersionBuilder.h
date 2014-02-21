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
#include <QMap>

class OneSixVersion;
class OneSixInstance;
class QWidget;
class QJsonObject;
class QFileInfo;
class VersionFile;

class OneSixVersionBuilder
{
	OneSixVersionBuilder();
public:
	static bool build(OneSixVersion *version, OneSixInstance *instance, QWidget *widgetParent, const bool onlyVanilla, const QStringList &external);
	static bool read(OneSixVersion *version, const QJsonObject &obj);
	static QMap<QString, int> readOverrideOrders(OneSixInstance *instance);
	static bool writeOverrideOrders(const QMap<QString, int> &order, OneSixInstance *instance);

	enum ParseFlag
	{
		NoFlags = 0x0,
		IsFTBPackJson = 0x1
	};
	Q_DECLARE_FLAGS(ParseFlags, ParseFlag)

private:
	OneSixVersion *m_version;
	OneSixInstance *m_instance;
	QWidget *m_widgetParent;

	bool build(const bool onlyVanilla, const QStringList &external);
	bool read(const QJsonObject &obj);

	bool read(const QFileInfo &fileInfo, const bool requireOrder, VersionFile *out, const ParseFlags flags = NoFlags);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(OneSixVersionBuilder::ParseFlags)
