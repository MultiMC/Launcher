// Licensed under the Apache-2.0 license. See README.md for details.

#include "AccountsWidget.h"
#include "ui_AccountsWidget.h"

#include "dialogs/AccountLoginDialog.h"
#include "dialogs/ProgressDialog.h"
#include "auth/BaseAccount.h"
#include "auth/BaseAccountType.h"
#include "auth/AccountModel.h"
#include "auth/DefaultAccountProxyModel.h"
#include "tasks/Task.h"
#include "BaseInstance.h"
#include "Env.h"
#include "resources/ResourceProxyModel.h"
#include "resources/Resource.h"
#include "MultiMC.h"

AccountsWidget::AccountsWidget(InstancePtr instance, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::AccountsWidget),
	m_instance(instance)
{
	ui->setupUi(this);

	ui->containerDefaultBtn->setVisible(!!m_instance);
	ui->useBtn->setVisible(!!m_instance);
	ui->progressWidget->setVisible(false);
	ui->cancelBtn->setText(m_instance ? tr("Cancel") : tr("Close"));

	DefaultAccountProxyModel *proxy = new DefaultAccountProxyModel(m_instance, this);
	proxy->setSourceModel(MMC->accountsModel().get());
	ui->view->setModel(ResourceProxyModel::mixin<QIcon>(proxy));
	connect(ui->view->selectionModel(), &QItemSelectionModel::currentChanged, this, &AccountsWidget::currentChanged);
	currentChanged(ui->view->currentIndex(), QModelIndex());

	connect(ui->cancelBtn, &QPushButton::clicked, this, &AccountsWidget::rejected);
}

AccountsWidget::~AccountsWidget()
{
	delete ui;
}

void AccountsWidget::setRequestedAccountType(const QString &type)
{
	m_requestedType = type;
	ui->useBtn->setVisible(true);
	currentChanged(ui->view->currentIndex(), QModelIndex());
}
void AccountsWidget::setSession(SessionPtr session)
{
	m_session = session;
}
void AccountsWidget::setCancelEnabled(const bool enableCancel)
{
	ui->cancelBtn->setVisible(enableCancel);
}

BaseAccount *AccountsWidget::account() const
{
	return MMC->accountsModel()->getAccount(ui->view->currentIndex());
}

void AccountsWidget::on_addBtn_clicked()
{
	if (m_requestedType.isEmpty())
	{
		AccountLoginDialog dlg(this);
		if (dlg.exec() == QDialog::Accepted)
		{
			MMC->accountsModel()->registerAccount(dlg.account());
		}
	}
	else
	{
		AccountLoginDialog dlg(m_requestedType, this);
		if (dlg.exec() == QDialog::Accepted)
		{
			MMC->accountsModel()->registerAccount(dlg.account());
		}
	}

}
void AccountsWidget::on_removeBtn_clicked()
{
	BaseAccount *account = MMC->accountsModel()->getAccount(ui->view->currentIndex());
	if (account)
	{
		Task *task = account->createLogoutTask(m_session);
		if (task)
		{
			ProgressDialog(this).exec(task);
		}
		MMC->accountsModel()->unregisterAccount(account);
	}
}
void AccountsWidget::on_containerDefaultBtn_toggled()
{
	BaseAccount *account = MMC->accountsModel()->getAccount(ui->view->currentIndex());
	if (account)
	{
		if (ui->containerDefaultBtn->isChecked())
		{
			MMC->accountsModel()->setInstanceDefault(m_instance, account);
		}
		else
		{
			MMC->accountsModel()->unsetDefault(account->type(), m_instance);
		}
	}
}
void AccountsWidget::on_globalDefaultBtn_toggled()
{
	BaseAccount *account = MMC->accountsModel()->getAccount(ui->view->currentIndex());
	if (account)
	{
		if (ui->globalDefaultBtn->isChecked())
		{
			MMC->accountsModel()->setGlobalDefault(account);
		}
		else
		{
			MMC->accountsModel()->unsetDefault(account->type());
		}
	}
}

void AccountsWidget::on_useBtn_clicked()
{
	BaseAccount *account = MMC->accountsModel()->getAccount(ui->view->currentIndex());
	if (account)
	{
		ui->groupBox->setEnabled(false);
		ui->useBtn->setEnabled(false);
		ui->view->setEnabled(false);
		ui->progressWidget->setVisible(true);
		std::shared_ptr<Task> task = std::shared_ptr<Task>(account->createCheckTask(m_session));
		ui->progressWidget->exec(task);
		ui->progressWidget->setVisible(false);

		if (task->successful())
		{
			emit accepted();
		}
		else
		{
			AccountLoginDialog dlg(account, this);
			if (dlg.exec() == AccountLoginDialog::Accepted)
			{
				emit accepted();
			}
			else
			{
				ui->groupBox->setEnabled(true);
				ui->useBtn->setEnabled(true);
				ui->view->setEnabled(true);
			}
		}
	}
}

void AccountsWidget::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
	if (!current.isValid())
	{
		ui->groupBox->setEnabled(false);
		ui->avatarLbl->setPixmap(QPixmap());
		ui->usernameLbl->setText("");
		ui->containerDefaultBtn->setText(tr("Default for %1 (%2)").arg("-", m_instance ? m_instance->name() : QString()));
		ui->globalDefaultBtn->setText(tr("Default for %1").arg("-"));
		ui->containerDefaultBtn->setChecked(false);
		ui->globalDefaultBtn->setChecked(false);
		ui->useBtn->setEnabled(false);
	}
	else
	{
		BaseAccount *account = MMC->accountsModel()->getAccount(current);
		ui->groupBox->setEnabled(true);
		Resource::create(account->bigAvatar())->applyTo(ui->avatarLbl)->placeholder(Resource::create("icon:hourglass"));
		ui->usernameLbl->setText(account->username());
		ui->containerDefaultBtn->setText(tr("Default for %1 (%2)").arg(MMC->accountsModel()->type(account->type())->text(), m_instance ? m_instance->name() : QString()));
		ui->globalDefaultBtn->setText(tr("Default for %1").arg(MMC->accountsModel()->type(account->type())->text()));
		ui->containerDefaultBtn->setChecked(m_instance && MMC->accountsModel()->isInstanceDefaultExplicit(m_instance, account));
		ui->globalDefaultBtn->setChecked(MMC->accountsModel()->isGlobalDefault(account));
		ui->useBtn->setEnabled(m_requestedType == account->type());
	}
}
