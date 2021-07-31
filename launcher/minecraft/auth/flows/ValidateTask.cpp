
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

#include "ValidateTask.h"
#include "../MinecraftAccount.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

#include <QDebug>

ValidateTask::ValidateTask(AccountData * data, QObject *parent)
    : YggdrasilTask(data, parent)
{
}

QJsonObject ValidateTask::getRequestContent() const
{
    QJsonObject req;
    req.insert("accessToken", m_data->accessToken());
    return req;
}

void ValidateTask::processResponse(QJsonObject responseData)
{
    // Assume that if processError wasn't called, then the request was successful.
    changeState(YggdrasilTask::STATE_SUCCEEDED);
}

QString ValidateTask::getEndpoint() const
{
    return "validate";
}

QString ValidateTask::getStateMessage() const
{
    switch (m_state)
    {
    case YggdrasilTask::STATE_WORKING:
        return tr("Validating access token...");
    default:
        return YggdrasilTask::getStateMessage();
    }
}
