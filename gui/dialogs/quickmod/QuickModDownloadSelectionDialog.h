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

#include <QDialog>
#include <memory>

namespace Ui
{
class QuickModDownloadSelectionDialog;
}

typedef std::shared_ptr<class QuickModVersion> QuickModVersionPtr;
class QuickModDownload;

class QuickModDownloadSelectionDialog : public QDialog
{
	Q_OBJECT

public:
	~QuickModDownloadSelectionDialog();

	static QuickModDownload select(const QuickModVersionPtr version, QWidget *widgetParent = 0);

private:
	explicit QuickModDownloadSelectionDialog(const QuickModVersionPtr version,
											 QWidget *parent = 0);
	Ui::QuickModDownloadSelectionDialog *ui;
	const QuickModVersionPtr m_version;

	int selectedIndex() const;

	static QuickModDownload highestPriorityDownload(const QuickModVersionPtr version,
													const int type = 0);
};
