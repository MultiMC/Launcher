/* Copyright 2015 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
