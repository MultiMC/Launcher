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

#include <QDialog>

namespace Ui
{
class QuickModAddFileDialog;
}

class QuickModAddFileDialog : public QDialog
{
	Q_OBJECT

public:
	explicit QuickModAddFileDialog(QWidget *parent = 0);
	~QuickModAddFileDialog();

	enum Type
	{
		FileName,
		Url
	};
	Type type() const
	{
		return m_type;
	}
	QString fileName() const;
	QUrl url() const;

private
slots:
	void on_addButton_clicked();
	void on_browseButton_clicked();

private:
	Ui::QuickModAddFileDialog *ui;
	Type m_type;
};
