// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include "tasks/StandardTask.h"
#include <memory>

class BaseAccount;
using InstancePtr = std::shared_ptr<class BaseInstance>;
using SessionPtr = std::shared_ptr<class BaseSession>;

class AuthTask : public StandardTask
{
	Q_OBJECT
public:
	AuthTask(const QString &type, InstancePtr instance, SessionPtr session, QObject *parent = nullptr);

	BaseAccount *account() const { return m_account; }

private:
	void executeTask() override;
	void abort() override;

	QString m_type;
	InstancePtr m_instance;
	BaseAccount *m_account;
	SessionPtr m_session;
};
