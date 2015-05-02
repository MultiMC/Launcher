// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QDialog>
#include <memory>

class AccountsWidget;
class BaseAccount;
using InstancePtr = std::shared_ptr<class BaseInstance>;
using SessionPtr = std::shared_ptr<class BaseSession>;

class AccountsDialog : public QDialog
{
	Q_OBJECT
public:
	explicit AccountsDialog(const QString &type, InstancePtr instance = nullptr, QWidget *parent = nullptr);
	~AccountsDialog();

	void setSession(SessionPtr session);

	BaseAccount *account() const;

private:
	AccountsWidget *m_widget;
	InstancePtr m_instance;
	SessionPtr m_session;
	QString m_requestedType;
};
