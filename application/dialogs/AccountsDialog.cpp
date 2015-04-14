// Licensed under the Apache-2.0 license. See README.md for details.

#include "AccountsDialog.h"

#include <QHBoxLayout>

#include "widgets/AccountsWidget.h"

AccountsDialog::AccountsDialog(const QString &type, InstancePtr instance, QWidget *parent) :
	QDialog(parent),
	m_instance(instance)
{
	m_widget = new AccountsWidget(type, instance, this);
	m_widget->setCancelEnabled(true);
	m_widget->setOfflineEnabled(!!m_instance, tr("Play Offline"));

	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->addWidget(m_widget);

	connect(m_widget, &AccountsWidget::accepted, this, &AccountsDialog::accept);
	connect(m_widget, &AccountsWidget::rejected, this, &AccountsDialog::reject);
}

AccountsDialog::~AccountsDialog()
{
}

void AccountsDialog::setSession(SessionPtr session)
{
	m_widget->setSession(session);
}
BaseAccount *AccountsDialog::account() const
{
	return m_widget->account();
}
