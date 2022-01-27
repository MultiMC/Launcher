/* Copyright 2013-2021 MultiMC Contributors
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

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QPair>
#include <QMap>
#include <QPixmap>

#include <memory>

#include "AuthSession.h"
#include "Usable.h"
#include "AccountData.h"
#include "QObjectPtr.h"

class Task;
class AccountTask;
class MinecraftAccount;

typedef shared_qobject_ptr<MinecraftAccount> MinecraftAccountPtr;
Q_DECLARE_METATYPE(MinecraftAccountPtr)

/**
 * A profile within someone's Mojang account.
 *
 * Currently, the profile system has not been implemented by Mojang yet,
 * but we might as well add some things for it in MultiMC right now so
 * we don't have to rip the code to pieces to add it later.
 */
struct AccountProfile
{
    QString id;
    QString name;
    bool legacy;
};

/**
 * Object that stores information about a certain Mojang account.
 *
 * Said information may include things such as that account's username, client token, and access
 * token if the user chose to stay logged in.
 */
class MinecraftAccount :
    public QObject,
    public Usable
{
    Q_OBJECT
public: /* construction */
    //! Do not copy accounts. ever.
    explicit MinecraftAccount(const MinecraftAccount &other, QObject *parent) = delete;

    //! Default constructor
    explicit MinecraftAccount(QObject *parent = 0);

    static MinecraftAccountPtr createFromUsername(const QString &username);

    static MinecraftAccountPtr createBlankMSA();

    static MinecraftAccountPtr loadFromJsonV2(const QJsonObject &json);
    static MinecraftAccountPtr loadFromJsonV3(const QJsonObject &json);

    //! Saves a MinecraftAccount to a JSON object and returns it.
    QJsonObject saveToJson() const;

public: /* manipulation */

    /**
     * Attempt to login. Empty password means we use the token.
     * If the attempt fails because we already are performing some task, it returns false.
     */
    shared_qobject_ptr<AccountTask> login(QString password);

    shared_qobject_ptr<AccountTask> loginMSA();

    shared_qobject_ptr<AccountTask> refresh();

    shared_qobject_ptr<AccountTask> currentTask();

public: /* queries */
    QString internalId() const {
        return data.internalId;
    }

    QString accountDisplayString() const {
        return data.accountDisplayString();
    }

    QString mojangUserName() const {
        return data.userName();
    }

    QString accessToken() const {
        return data.accessToken();
    }

    QString profileId() const {
        return data.profileId();
    }

    QString profileName() const {
        return data.profileName();
    }

    bool isActive() const;

    bool canMigrate() const {
        return data.canMigrateToMSA;
    }

    bool isMSA() const {
        return data.type == AccountType::MSA;
    }

    bool ownsMinecraft() const {
        return data.minecraftEntitlement.ownsMinecraft;
    }

    bool hasProfile() const {
        return data.profileId().size() != 0;
    }

    QString typeString() const {
        switch(data.type) {
            case AccountType::Mojang: {
                if(data.legacy) {
                    return "legacy";
                }
                return "mojang";
            }
            break;
            case AccountType::MSA: {
                return "msa";
            }
            break;
            default: {
                return "unknown";
            }
        }
    }

    QPixmap getFace() const;

    //! Returns the current state of the account
    AccountState accountState() const;

    AccountData * accountData() {
        return &data;
    }

    bool shouldRefresh() const;

    void fillSession(AuthSessionPtr session);

    QString lastError() const {
        return data.lastError();
    }

signals:
    /**
     * This signal is emitted when the account changes
     */
    void changed();

    void activityChanged(bool active);

    // TODO: better signalling for the various possible state changes - especially errors

protected: /* variables */
    AccountData data;

    // current task we are executing here
    shared_qobject_ptr<AccountTask> m_currentTask;

protected: /* methods */

    void incrementUses() override;
    void decrementUses() override;

private
slots:
    void authSucceeded();
    void authFailed(QString reason);
};
