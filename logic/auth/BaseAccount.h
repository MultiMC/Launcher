// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QString>
#include <QMap>
#include <QMetaType>
#include <memory>
#include "BaseSession.h"

class Task;
class QJsonObject;
using AccountModelPtr = std::shared_ptr<class AccountModel>;

class BaseAccount
{
public:
	explicit BaseAccount(AccountModelPtr model);
	virtual ~BaseAccount() {}

	virtual QString avatar() const { return QString(); }
	virtual QString bigAvatar() const { return avatar(); }

	virtual QString type() const = 0;

	virtual Task *createLoginTask(const QString &username, const QString &password, SessionPtr session = nullptr) = 0;
	virtual Task *createCheckTask(SessionPtr session = nullptr) = 0;
	virtual Task *createLogoutTask(SessionPtr session = nullptr) = 0;

	QString username() const { return m_username; }
	void setUsername(const QString &username);

	// can be different from username. username is used for display etc., loginUsername for giving the username edit a default value
	virtual QString loginUsername() const { return username(); }

	bool hasToken(const QString &key) const { return m_tokens.contains(key); }
	QString token(const QString &key) const { return m_tokens[key]; }
	void setToken(const QString &key, const QString &token);

	virtual void load(const QJsonObject &obj);
	virtual QJsonObject save() const;

protected:
	void changed();

private:
	QString m_username;
	QMap<QString, QString> m_tokens;
	AccountModelPtr m_model;
};
Q_DECLARE_METATYPE(BaseAccount *)
