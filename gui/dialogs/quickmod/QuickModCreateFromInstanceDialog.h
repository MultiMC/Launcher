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
class QuickModCreateFromInstanceDialog;
}

class OneSixInstance;
class ProgressProvider;
class QuickModVersionBuilder;
class QuickModVersionRef;
typedef std::shared_ptr<class QuickModVersion> QuickModVersionPtr;

class QStringListModel;

class QuickModCreateFromInstanceDialog : public QDialog
{
	Q_OBJECT

public:
	explicit QuickModCreateFromInstanceDialog(std::shared_ptr<OneSixInstance> instance,
											  QWidget *parent = 0);
	~QuickModCreateFromInstanceDialog();

private slots:
	void on_browseBtn_clicked();

	void on_remoteBtn_clicked();
	void on_localBtn_clicked();

	void on_exportBtn_clicked();
	void on_importBtn_clicked();

	void on_authorsAddBtn_clicked();
	void on_authorsRemoveBtn_clicked();
	void on_tagsAddBtn_clicked();
	void on_tagsRemoveBtn_clicked();
	void on_categoriesAddBtn_clicked();
	void on_categoriesRemoveBtn_clicked();
	void on_urlsAddBtn_clicked();
	void on_urlsRemoveBtn_clicked();

	void on_endpointEdit_textChanged(const QString &text);

	void resetValues();
	void resetTables();

private:
	Ui::QuickModCreateFromInstanceDialog *ui;

	std::shared_ptr<OneSixInstance> m_instance;
	QList<QuickModVersionPtr> m_otherVersions;
	bool hasVersion(const QString &name) const;
	void createVersion(QuickModVersionBuilder builder) const;

	QStringListModel *m_tagsModel;
	QStringListModel *m_categoriesModel;

	void start(ProgressProvider *provider);

	QString filename() const;
	QByteArray toJson() const;
	void fromJson(const QByteArray &json);

	static QStringList reverseStringList(const QStringList &list);
	static QStringList variantListToStringList(const QVariantList &list);
};
