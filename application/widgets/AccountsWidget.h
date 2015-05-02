// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QWidget>
#include <memory>

namespace Ui {
class AccountsWidget;
}

class BaseAccount;
using InstancePtr = std::shared_ptr<class BaseInstance>;
using SessionPtr = std::shared_ptr<class BaseSession>;

class AccountsWidget : public QWidget
{
	Q_OBJECT
public:
	explicit AccountsWidget(const QString &type, InstancePtr instance = nullptr, QWidget *parent = nullptr);
	~AccountsWidget();

	void setSession(SessionPtr session);
	void setCancelEnabled(const bool enableCancel);
	void setOfflineEnabled(const bool enabled, const QString &text);

	BaseAccount *account() const;

signals:
	void accepted();
	void rejected();

private slots:
	void on_addBtn_clicked();
	void on_removeBtn_clicked();
	void on_containerDefaultBtn_toggled();
	void on_globalDefaultBtn_toggled();
	void on_useBtn_clicked();
	void on_offlineBtn_clicked();
	void currentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
	Ui::AccountsWidget *ui;
	InstancePtr m_instance;
	SessionPtr m_session;
	QString m_requestedType;
	bool m_offlineEnabled = false;
};
