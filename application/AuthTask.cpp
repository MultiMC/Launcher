// Licensed under the Apache-2.0 license. See README.md for details.

#include "AuthTask.h"

#include "auth/AccountModel.h"
#include "auth/BaseAccount.h"
#include "BaseInstance.h"
#include "dialogs/AccountLoginDialog.h"
#include "dialogs/AccountsDialog.h"
#include "Exception.h"
#include "MultiMC.h"

AuthTask::AuthTask(const QString &type, InstancePtr instance, SessionPtr session, QObject *parent)
	: StandardTask(parent), m_type(type), m_instance(instance), m_session(session)
{
}

void AuthTask::executeTask()
{
	BaseAccount *account = MMC->accountsModel()->get(m_type, m_instance);
	if (account)
	{
		try
		{
			setStatus(tr("Checking if default account is valid..."));
			runTask(account->createCheckTask(m_session));
			// easiest: have default and it's valid
			m_account = account;
			emitSucceeded();
			return;
		}
		catch (...)
		{
			setStatus(tr("Querying user for authentication details..."));
			emit progress(101, 100);
			AccountLoginDialog dlg(account);
			dlg.setSession(m_session);
			if (dlg.exec() == QDialog::Accepted)
			{
				m_account = account;
				emitSucceeded();
				return;
			}
			else
			{
				emitFailed(tr("No valid authentication details given"));
			}
		}
	}
	else
	{
		setStatus(tr("Querying user for account..."));
		AccountsDialog dlg(m_instance);
		dlg.setSession(m_session);
		dlg.setRequestedAccountType(m_type);
		if (dlg.exec() != QDialog::Accepted)
		{
			emitFailed(tr("No account choosen"));
			return;
		}
		BaseAccount *acc = dlg.account();
		if (acc)
		{
			m_account = acc;
			emitSucceeded();
			return;
		}
		else
		{
			emitFailed(tr("No account choosen"));
		}
	}
}
void AuthTask::abort()
{
}
