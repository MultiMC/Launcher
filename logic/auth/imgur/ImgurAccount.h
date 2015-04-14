// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include "auth/BaseAccount.h"
#include "auth/BaseAccountType.h"

class ImgurAccount : public BaseAccount
{
public:
	explicit ImgurAccount();

	QString type() const override { return "imgur"; }

	Task *createLoginTask(const QString &username, const QString &password) override;
	Task *createCheckTask() override;
	Task *createLogoutTask() override;
};

class ImgurAccountType : public BaseAccountType
{
public:
	QString id() const override { return "imgur"; }
	QString text() const override;
	QString icon() const override { return "imgur"; }
	QString usernameText() const override;
	QString passwordText() const override { return QString(); }
	Type type() const override { return OAuth2Pin; }
	QUrl oauth2PinUrl() const override;
	BaseAccount *createAccount() override { return new ImgurAccount; }
};
