// Licensed under the Apache-2.0 license. See README.md for details.

#include "AccountLoginDialog.h"
#include "ui_AccountLoginDialog.h"

#include "auth/AccountModel.h"
#include "auth/BaseAccount.h"
#include "auth/BaseAccountType.h"
#include "tasks/Task.h"
#include "IconRegistry.h"
#include "IconProxyModel.h"
#include "MultiMC.h"

AccountLoginDialog::AccountLoginDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AccountLoginDialog)
{
	setWindowTitle(tr("New Account"));
	ui->setupUi(this);
	ui->progressWidget->setVisible(false);
	ui->errorLbl->setVisible(false);
	ui->loginBtn->setFocus();
	ui->usernameEdit->setFocus();
	ui->typeBox->setModel(IconProxyModel::mixin(MMC->accountsModel()->typesModel()));

	connect(ui->typeBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AccountLoginDialog::currentTypeChanged);
	currentTypeChanged(0);
}
AccountLoginDialog::AccountLoginDialog(const QString &type, QWidget *parent)
	: AccountLoginDialog(parent)
{
	setWindowTitle(tr("New Account for %1").arg(MMC->accountsModel()->type(type)->text()));
	ui->typeBox->setCurrentIndex(ui->typeBox->findData(type));
	ui->typeBox->setDisabled(true);

	setupForType(type);
}
AccountLoginDialog::AccountLoginDialog(BaseAccount *account, QWidget *parent)
	: AccountLoginDialog(account->type(), parent)
{
	setWindowTitle(tr("Login"));
	m_account = account;
	ui->usernameEdit->setText(account->loginUsername());
}

AccountLoginDialog::~AccountLoginDialog()
{
	delete ui;
}

void AccountLoginDialog::setSession(SessionPtr session)
{
	m_session = session;
}

void AccountLoginDialog::on_cancelBtn_clicked()
{
	reject();
}
void AccountLoginDialog::on_loginBtn_clicked()
{
	std::shared_ptr<Task> task = std::shared_ptr<Task>(m_account->createLoginTask(ui->usernameEdit->text(), ui->passwordEdit->text(), m_session));
	connect(task.get(), &Task::finished, this, &AccountLoginDialog::taskFinished);
	ui->typeBox->setEnabled(false);
	ui->errorLbl->setVisible(false);
	ui->usernameEdit->setEnabled(false);
	ui->passwordEdit->setEnabled(false);
	ui->loginBtn->setEnabled(false);
	ui->progressWidget->setVisible(true);
	ui->progressWidget->start(task);
}

void AccountLoginDialog::taskFinished()
{
	Task *task = qobject_cast<Task *>(sender());
	if (task->successful())
	{
		accept();
	}
	else
	{
		ui->typeBox->setEnabled(true);
		ui->errorLbl->setVisible(true);
		ui->errorLbl->setText(task->failReason());
		ui->usernameEdit->setEnabled(true);
		ui->passwordEdit->setEnabled(true);
		ui->loginBtn->setEnabled(true);
		ui->progressWidget->setVisible(false);
	}
}

void AccountLoginDialog::currentTypeChanged(const int index)
{
	const QString type = ui->typeBox->itemData(index).toString();
	if (!type.isEmpty())
	{
		setupForType(type);
	}
}
void AccountLoginDialog::setupForType(const QString &type)
{
	const BaseAccountType *t = MMC->accountsModel()->type(type);
	ui->usernameLbl->setText(t->usernameText());
	ui->usernameLbl->setVisible(!t->usernameText().isNull());
	ui->usernameEdit->setVisible(!t->usernameText().isNull());
	ui->passwordLbl->setText(t->passwordText());
	ui->passwordLbl->setVisible(!t->passwordText().isNull());
	ui->passwordEdit->setVisible(!t->passwordText().isNull());
	if (t->type() == BaseAccountType::OAuth2Pin)
	{
		ui->infoLbl->setVisible(true);
		ui->infoLbl->setText(tr("To authorize, go to <a href=\"%1\">%1</a> and follow the instructions. You will receive a PIN code which you need to enter below.")
							 .arg(t->oauth2PinUrl().toString()));
	}
	else
	{
		ui->infoLbl->setVisible(false);
	}
	if (m_account && m_account->type() != type)
	{
		delete m_account;
		m_account = nullptr;
	}
	if (!m_account)
	{
		m_account = MMC->accountsModel()->type(type)->createAccount(MMC->accountsModel());
	}
}
