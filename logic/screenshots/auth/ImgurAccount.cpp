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

#include "ImgurAccount.h"

#include "ImgurAuthTask.h"
#include "Env.h"

ImgurAccount::ImgurAccount(BaseAccountType *type)
	: BaseAccount(type)
{
}

Task *ImgurAccount::createLoginTask(const QString &username, const QString &password, SessionPtr session)
{
	return new ImgurAuthenticateTask(username, this);
}
Task *ImgurAccount::createCheckTask(SessionPtr session)
{
	return new ImgurValidateTask(this);
}
Task *ImgurAccount::createLogoutTask(SessionPtr session)
{
	return nullptr;
}

QString ImgurAccountType::text() const
{
	return QObject::tr("Imgur");
}
QString ImgurAccountType::usernameText() const
{
	return QObject::tr("PIN");
}

QUrl ImgurAccountType::oauth2PinUrl() const
{
	return QUrl(QString("https://api.imgur.com/oauth2/authorize?client_id=%1&response_type=pin&state=")
				.arg(ENV.configuration()["ImgurClientID"].toString()));
}

bool ImgurAccountType::isAvailable() const
{
	return !ENV.configuration()["ImgurClientID"].toString().isEmpty();
}
