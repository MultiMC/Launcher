// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QDialog>
#include <memory>

namespace Ui {
class AccountLoginDialog;
}

class BaseAccount;
using SessionPtr = std::shared_ptr<class BaseSession>;

class AccountLoginDialog : public QDialog
{
	Q_OBJECT
public:
	explicit AccountLoginDialog(QWidget *parent = nullptr);
	explicit AccountLoginDialog(const QString &type, QWidget *parent = nullptr);
	explicit AccountLoginDialog(BaseAccount *account, QWidget *parent = nullptr);
	~AccountLoginDialog();

	void setSession(SessionPtr session);

	BaseAccount *account() const { return m_account; }

private slots:
	void on_cancelBtn_clicked();
	void on_loginBtn_clicked();
	void currentTypeChanged(const int index);

	void taskFinished();

private:
	Ui::AccountLoginDialog *ui;
	BaseAccount *m_account = nullptr;
	SessionPtr m_session;

	void setupForType(const QString &type);
};
