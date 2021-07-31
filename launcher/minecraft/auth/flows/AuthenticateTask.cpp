
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

#include "AuthenticateTask.h"
#include "../MinecraftAccount.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

#include <QDebug>
#include <QUuid>

AuthenticateTask::AuthenticateTask(AccountData * data, const QString &password, QObject *parent)
    : YggdrasilTask(data, parent), m_password(password)
{
}

QJsonObject AuthenticateTask::getRequestContent() const
{
    /*
     * {
     *   "agent": {                                // optional
     *   "name": "Minecraft",                    // So far this is the only encountered value
     *   "version": 1                            // This number might be increased
     *                                             // by the vanilla client in the future
     *   },
     *   "username": "mojang account name",        // Can be an email address or player name for
                                                // unmigrated accounts
     *  "password": "mojang account password",
     *  "clientToken": "client identifier"        // optional
     *  "requestUser": true/false               // request the user structure
     * }
     */
    QJsonObject req;

    {
        QJsonObject agent;
        // C++ makes string literals void* for some stupid reason, so we have to tell it
        // QString... Thanks Obama.
        agent.insert("name", QString("Minecraft"));
        agent.insert("version", 1);
        req.insert("agent", agent);
    }

    req.insert("username", m_data->userName());
    req.insert("password", m_password);
    req.insert("requestUser", false);

    // If we already have a client token, give it to the server.
    // Otherwise, let the server give us one.

    m_data->generateClientTokenIfMissing();
    req.insert("clientToken", m_data->clientToken());

    return req;
}

void AuthenticateTask::processResponse(QJsonObject responseData)
{
    // Read the response data. We need to get the client token, access token, and the selected
    // profile.
    qDebug() << "Processing authentication response.";
    // qDebug() << responseData;
    // If we already have a client token, make sure the one the server gave us matches our
    // existing one.
    qDebug() << "Getting client token.";
    QString clientToken = responseData.value("clientToken").toString("");
    if (clientToken.isEmpty())
    {
        // Fail if the server gave us an empty client token
        changeState(STATE_FAILED_HARD, tr("Authentication server didn't send a client token."));
        return;
    }
    if(m_data->clientToken().isEmpty()) {
        m_data->setClientToken(clientToken);
    }
    else if(clientToken != m_data->clientToken()) {
        changeState(STATE_FAILED_HARD, tr("Authentication server attempted to change the client token. This isn't supported."));
        return;
    }

    // Now, we set the access token.
    qDebug() << "Getting access token.";
    QString accessToken = responseData.value("accessToken").toString("");
    if (accessToken.isEmpty())
    {
        // Fail if the server didn't give us an access token.
        changeState(STATE_FAILED_HARD, tr("Authentication server didn't send an access token."));
        return;
    }
    // Set the access token.
    m_data->yggdrasilToken.token = accessToken;
    m_data->yggdrasilToken.validity = Katabasis::Validity::Certain;

    // Now we load the list of available profiles.
    qDebug() << "Loading profile list.";
    QJsonArray availableProfiles = responseData.value("availableProfiles").toArray();
    if(availableProfiles.size() < 1) {
        changeState(STATE_FAILED_HARD, tr("Authentication server didn't return any profile. The account exists, but likely isn't premium."));
        return;
    }
    if(availableProfiles.size() > 1) {
        changeState(STATE_FAILED_HARD, tr("Authentication server specified multiple profiles. Please contact Mojang support to fix this problem."));
        return;
    }

    // get the profile data
    QJsonObject profile = availableProfiles[0].toObject();
    m_data->minecraftProfile.id = profile.value("id").toString("");
    m_data->minecraftProfile.name = profile.value("name").toString("");
    m_data->legacy = profile.value("legacy").toBool(false);
    m_data->minecraftProfile.validity = Katabasis::Validity::Certain;

    m_data->validity_ = Katabasis::Validity::Certain;

    // We've made it through the minefield of possible errors. Return true to indicate that
    // we've succeeded.
    qDebug() << "Finished reading authentication response.";
    changeState(STATE_SUCCEEDED);
}

QString AuthenticateTask::getEndpoint() const
{
    return "authenticate";
}

QString AuthenticateTask::getStateMessage() const
{
    switch (m_state)
    {
    case STATE_WORKING:
        return tr("Authenticating...");
    default:
        return YggdrasilTask::getStateMessage();
    }
}
