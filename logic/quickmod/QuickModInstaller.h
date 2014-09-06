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
#include <memory>

class QuickModVersion;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;
class BaseInstance;
typedef std::shared_ptr<BaseInstance> InstancePtr;

/**
 * Non-gui backend for QuickModInstallDialog
 */
class QuickModInstaller : public QObject
{
	Q_OBJECT
public:
	explicit QuickModInstaller(QWidget *widgetParent, QObject *parent = 0);

	void install(const QuickModVersionPtr version, InstancePtr instance);

	void handleDownload(QuickModVersionPtr version, const QByteArray &data, const QUrl &url);

private:
	QWidget *m_widgetParent;
};
