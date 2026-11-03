/* Copyright 2015-2021 MultiMC Contributors
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

#include <QElapsedTimer>
#include <QJsonObject>
#include <QObject>
#include "QObjectPtr.h"

#include "net/Mode.h"
#include "net/NetJob.h"

namespace Meta
{
struct UpdateState;

class BaseEntity
{
public: /* types */
    using Ptr = std::shared_ptr<BaseEntity>;
    enum class LoadStatus
    {
        NotLoaded,
        Local,
        Remote
    };
    enum class UpdateStatus
    {
        NotDone,
        InProgress,
        Failed,
        Succeeded
    };

public:
    virtual ~BaseEntity();

    virtual void parse(const QJsonObject &obj) = 0;

    virtual QString localFilename() const = 0;
    virtual QUrl url() const;

    bool isLoaded() const;
    bool shouldStartRemoteUpdate() const;

    void load(Net::Mode loadType);
    Task::Ptr getCurrentTask();

    /// The SHA-256 the parent lists for this, empty if it doesn't list one
    QByteArray sha256() const;
    /// Whether what's loaded is what the parent lists. Stale data still loads, it's just not current.
    bool isCurrent() const;

public: /* for the parent and the parsers only */
    void setSha256(const QByteArray &sha256);
    /// The entity listing this one's hash: nullptr for the root index
    void setParentEntity(BaseEntity *parent);

protected: /* methods */
    bool loadLocalFile();

private: /* methods */
    friend class ParsingValidator;
    class UpdateTask;
    bool parseData(const QByteArray &data, const QString &what);
    void startUpdate(bool force);
    BaseEntity *rootEntity();
    /// the root's part of UpdateState, from this entity as the root
    UpdateState rootState() const;

private:
    LoadStatus m_loadStatus = LoadStatus::NotLoaded;
    UpdateStatus m_updateStatus = UpdateStatus::NotDone;
    Task::Ptr m_updateTask;
    BaseEntity *m_parent = nullptr;
    QByteArray m_sha256;
    QByteArray m_loadedSha256;
    // when the root was last fetched, and when fetching it last failed
    QElapsedTimer m_fetched;
    QElapsedTimer m_fetchFailed;
};
}
