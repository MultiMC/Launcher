// Licensed under the Apache-2.0 license. See README.md for details.

#include "AccountLoginDialog.h"
#include "ui_AccountLoginDialog.h"

#include <QPushButton>

#include "auth/AccountModel.h"
#include "auth/BaseAccount.h"
#include "auth/BaseAccountType.h"
#include "tasks/Task.h"
#include "resources/ResourceProxyModel.h"
#include "MultiMC.h"

AccountLoginDialog::AccountLoginDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AccountLoginDialog)
{
	setWindowTitle(tr("New Account"));
	ui->setupUi(this);
	ui->progressWidget->setVisible(false);
	ui->errorLbl->setVisible(false);
	ui->userTextBox->setFocus();
	ui->typeBox->setModel(ResourceProxyModel::mixin<QIcon>(MMC->accountsModel()->typesModel()));

	connect(ui->typeBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AccountLoginDialog::currentTypeChanged);
	currentTypeChanged(0);

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AccountLoginDialog::loginClicked);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AccountLoginDialog::reject);
}
AccountLoginDialog::AccountLoginDialog(BaseAccountType *type, QWidget *parent)
	: AccountLoginDialog(parent)
{
	setWindowTitle(tr("New Account for %1").arg(type->text()));
	ui->typeBox->setCurrentIndex(ui->typeBox->findData(QVariant::fromValue<BaseAccountType *>(type)));
	ui->typeBox->setDisabled(true);

	setupForType(type);
}
AccountLoginDialog::AccountLoginDialog(BaseAccount *account, QWidget *parent)
	: AccountLoginDialog(account->type(), parent)
{
	setWindowTitle(tr("Login"));
	m_account = account;
	ui->userTextBox->setVisible(false);
}

AccountLoginDialog::~AccountLoginDialog()
{
	delete ui;
}

void AccountLoginDialog::setSession(SessionPtr session)
{
	m_session = session;
}

void AccountLoginDialog::loginClicked()
{
	std::shared_ptr<Task> task = std::shared_ptr<Task>(m_account->createLoginTask(ui->userTextBox->text(), ui->passTextBox->text(), m_session));
	connect(task.get(), &Task::finished, this, &AccountLoginDialog::taskFinished);
	ui->typeBox->setEnabled(false);
	ui->errorLbl->setVisible(false);
	ui->userTextBox->setEnabled(false);
	ui->passTextBox->setEnabled(false);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
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
		ui->typeBox->setEnabled(m_type);
		ui->errorLbl->setVisible(true);
		ui->errorLbl->setText(task->failReason());
		ui->userTextBox->setEnabled(true);
		ui->passTextBox->setEnabled(true);
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
		ui->progressWidget->setVisible(false);
	}
}

void AccountLoginDialog::currentTypeChanged(const int index)
{
	BaseAccountType *type = ui->typeBox->itemData(index).value<BaseAccountType *>();
	if (type)
	{
		setupForType(type);
	}
}
void AccountLoginDialog::setupForType(BaseAccountType *type)
{
	m_type = type;

	ui->userTextBox->setPlaceholderText(type->usernameText());
	ui->userTextBox->setVisible(!type->usernameText().isNull());
	ui->passTextBox->setPlaceholderText(type->passwordText());
	ui->passTextBox->setVisible(!type->passwordText().isNull());
	if (type->type() == BaseAccountType::OAuth2Pin)
	{
		ui->infoLbl->setVisible(true);
		ui->infoLbl->setText(tr("To authorize, go to <a href=\"%1\">%1</a> and follow the instructions. You will receive a PIN code which you need to enter below.")
							 .arg(type->oauth2PinUrl().toString()));
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
		m_account = MMC->accountsModel()->createAccount(type);
	}
}
