// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include "tasks/StandardTask.h"

class BaseAccount;

class ImgurAuthenticateTask : public StandardTask
{
public:
	explicit ImgurAuthenticateTask(const QString &pin, BaseAccount *account, QObject *parent = nullptr);

private:
	void start() override;

	QString m_pin;
	BaseAccount *m_account;
};
class ImgurValidateTask : public StandardTask
{
public:
	explicit ImgurValidateTask(BaseAccount *account, QObject *parent = nullptr);

private:
	void start() override;

	BaseAccount *m_account;
};
