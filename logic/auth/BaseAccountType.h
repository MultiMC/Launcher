// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QUrl>
#include <memory>

class BaseAccount;
class QString;
using AccountModelPtr = std::weak_ptr<class AccountModel>;

class BaseAccountType
{
public:
	virtual ~BaseAccountType() {}

	enum Type
	{
		OAuth2Pin,
		UsernamePassword
	};

	virtual QString id() const = 0;
	virtual QString text() const = 0;
	virtual QString icon() const = 0;
	virtual QString usernameText() const = 0;
	virtual QString passwordText() const = 0;
	virtual Type type() const = 0;
	virtual QUrl oauth2PinUrl() const { return QUrl(); }

	virtual BaseAccount *createAccount(AccountModelPtr model) = 0;
};
