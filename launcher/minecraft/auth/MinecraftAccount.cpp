/* Copyright 2013-2021 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
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

#include "MinecraftAccount.h"
#include "flows/RefreshTask.h"
#include "flows/AuthenticateTask.h"

#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegExp>
#include <QStringList>
#include <QJsonDocument>

#include <QDebug>

MinecraftAccountPtr MinecraftAccount::loadFromJsonV2(const QJsonObject& json) {
    MinecraftAccountPtr account(new MinecraftAccount());
    if(account->data.resumeStateFromV2(json)) {
        return account;
    }
    return nullptr;
}

MinecraftAccountPtr MinecraftAccount::loadFromJsonV3(const QJsonObject& json) {
    MinecraftAccountPtr account(new MinecraftAccount());
    if(account->data.resumeStateFromV3(json)) {
        return account;
    }
    return nullptr;
}

MinecraftAccountPtr MinecraftAccount::createFromUsername(const QString &username)
{
    MinecraftAccountPtr account(new MinecraftAccount());
    account->data.type = AccountType::Mojang;
    account->data.yggdrasilToken.extra["userName"] = username;
    account->data.yggdrasilToken.extra["clientToken"] = QUuid::createUuid().toString().remove(QRegExp("[{}-]"));
    return account;
}

QJsonObject MinecraftAccount::saveToJson() const
{
    return data.saveState();
}

AccountStatus MinecraftAccount::accountStatus() const {
    // FIXME: this needs to understand MSA
    if (accessToken().isEmpty())
        return NotVerified;
    else
        return Verified;
}

std::shared_ptr<YggdrasilTask> MinecraftAccount::login(AuthSessionPtr session, QString password)
{
    Q_ASSERT(m_currentTask.get() == nullptr);

    // take care of the true offline status
    if (accountStatus() == NotVerified && password.isEmpty())
    {
        if (session)
        {
            session->status = AuthSession::RequiresPassword;
            fillSession(session);
        }
        return nullptr;
    }

    if(accountStatus() == Verified && !session->wants_online)
    {
        session->status = AuthSession::PlayableOffline;
        session->auth_server_online = false;
        fillSession(session);
        return nullptr;
    }
    else
    {
        if (password.isEmpty())
        {
            m_currentTask.reset(new RefreshTask(&data));
        }
        else
        {
            m_currentTask.reset(new AuthenticateTask(&data, password));
        }
        m_currentTask->assignSession(session);

        connect(m_currentTask.get(), SIGNAL(succeeded()), SLOT(authSucceeded()));
        connect(m_currentTask.get(), SIGNAL(failed(QString)), SLOT(authFailed(QString)));
    }
    return m_currentTask;
}

void MinecraftAccount::authSucceeded()
{
    auto session = m_currentTask->getAssignedSession();
    if (session)
    {
        session->status =
            session->wants_online ? AuthSession::PlayableOnline : AuthSession::PlayableOffline;
        fillSession(session);
        session->auth_server_online = true;
    }
    m_currentTask.reset();
    emit changed();
}

void MinecraftAccount::authFailed(QString reason)
{
    auto session = m_currentTask->getAssignedSession();
    // This is emitted when the yggdrasil tasks time out or are cancelled.
    // -> we treat the error as no-op
    if (m_currentTask->state() == YggdrasilTask::STATE_FAILED_SOFT)
    {
        if (session)
        {
            session->status = accountStatus() == Verified ? AuthSession::PlayableOffline : AuthSession::RequiresPassword;
            session->auth_server_online = false;
            fillSession(session);
        }
    }
    else
    {
        data.yggdrasilToken.token = QString();
        data.yggdrasilToken.validity = Katabasis::Validity::None;
        data.validity_ = Katabasis::Validity::None;
        emit changed();
        if (session)
        {
            session->status = AuthSession::RequiresPassword;
            session->auth_server_online = true;
            fillSession(session);
        }
    }
    m_currentTask.reset();
}

void MinecraftAccount::fillSession(AuthSessionPtr session)
{
    // the user name. you have to have an user name
    // FIXME: not with MSA
    session->username = username();
    // volatile auth token
    session->access_token = accessToken();
    // the semi-permanent client token
    session->client_token = data.clientToken();
    // profile name
    session->player_name = profileName();
    // profile ID
    session->uuid = profileId();
    // 'legacy' or 'mojang', depending on account type
    session->user_type = typeString();
    if (!session->access_token.isEmpty())
    {
        session->session = "token:" + accessToken() + ":" + profileId();
    }
    else
    {
        session->session = "-";
    }
    session->m_accountPtr = shared_from_this();
}

void MinecraftAccount::decrementUses()
{
    Usable::decrementUses();
    if(!isInUse())
    {
        emit changed();
        // FIXME: we now need a better way to identify accounts...
        qWarning() << "Account" << username() << "is no longer in use.";
    }
}

void MinecraftAccount::incrementUses()
{
    bool wasInUse = isInUse();
    Usable::incrementUses();
    if(!wasInUse)
    {
        emit changed();
        // FIXME: we now need a better way to identify accounts...
        qWarning() << "Account" << username() << "is now in use.";
    }
}
