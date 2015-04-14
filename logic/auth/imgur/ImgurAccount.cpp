// Licensed under the Apache-2.0 license. See README.md for details.

#include "ImgurAccount.h"

#include "ImgurAuthTask.h"

ImgurAccount::ImgurAccount()
	: BaseAccount()
{
}

Task *ImgurAccount::createLoginTask(const QString &username, const QString &password)
{
	return new ImgurAuthenticateTask(username, this);
}
Task *ImgurAccount::createCheckTask()
{
	return new ImgurValidateTask(this);
}
Task *ImgurAccount::createLogoutTask()
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
				.arg(MMC_IMGUR_CLIENT_ID));
}
