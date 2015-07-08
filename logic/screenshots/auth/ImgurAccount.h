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

#include "auth/BaseAccount.h"
#include "auth/BaseAccountType.h"

class ImgurAccount : public BaseAccount
{
public:
	explicit ImgurAccount(BaseAccountType *type);

	Task *createLoginTask(const QString &username, const QString &password, SessionPtr session = nullptr) override;
	Task *createCheckTask(SessionPtr session = nullptr) override;
	Task *createLogoutTask(SessionPtr session = nullptr) override;
};

class ImgurAccountType : public BaseAccountType
{
public:
	QString text() const override;
	QString icon() const override { return "icon:imgur"; }
	QString usernameText() const override;
	QString passwordText() const override { return QString(); }
	Type type() const override { return OAuth2Pin; }
	QUrl oauth2PinUrl() const override;
	bool isAvailable() const override;
};
