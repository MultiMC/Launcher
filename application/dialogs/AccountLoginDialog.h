/* Copyright 2015 MultiMC Contributors
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

namespace Ui {
class AccountLoginDialog;
}

class BaseAccount;
class BaseAccountType;
using SessionPtr = std::shared_ptr<class BaseSession>;

class AccountLoginDialog : public QDialog
{
	Q_OBJECT
public:
	explicit AccountLoginDialog(QWidget *parent = nullptr);
	explicit AccountLoginDialog(BaseAccountType *type, QWidget *parent = nullptr);
	explicit AccountLoginDialog(BaseAccount *account, QWidget *parent = nullptr);
	~AccountLoginDialog();

	void setSession(SessionPtr session);

	BaseAccount *account() const { return m_account; }

private slots:
	void loginClicked();
	void currentTypeChanged(const int index);

	void taskFinished();

private:
	Ui::AccountLoginDialog *ui;
	BaseAccount *m_account = nullptr;
	SessionPtr m_session;
	BaseAccountType *m_type;

	void setupForType(BaseAccountType *type);
};
