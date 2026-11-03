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

#include "BaseEntity.h"

#include <QCryptographicHash>

#include "net/ChecksumValidator.h"
#include "net/Download.h"
#include "net/HttpMetaCache.h"
#include "net/NetJob.h"
#include "Json.h"
#include "UpdateStep.h"

#include "BuildConfig.h"
#include "Application.h"

namespace Meta
{
class ParsingValidator : public Net::Validator
{
public: /* con/des */
    ParsingValidator(BaseEntity *entity) : m_entity(entity)
    {
    };
    virtual ~ParsingValidator()
    {
    };

public: /* methods */
    bool init(QNetworkRequest &) override
    {
        // a retry or the next mirror starts over
        data.clear();
        return true;
    }
    bool write(QByteArray & data) override
    {
        this->data.append(data);
        return true;
    }
    bool abort() override
    {
        return true;
    }
    bool validate(QNetworkReply &) override
    {
        return m_entity->parseData(data, m_entity->localFilename());
    }

private: /* data */
    QByteArray data;
    BaseEntity *m_entity;
};

/*
 * Brings an entity up to date: its parent first, as that says what is current, then the entity
 * itself, if what it has doesn't match its hash. What to do at each point is decided by
 * nextUpdateStep(); this only waits for things and does what it says.
 */
class BaseEntity::UpdateTask : public Task
{
public:
    UpdateTask(BaseEntity *entity, bool forced) : m_entity(entity), m_forced(forced)
    {
    }

protected:
    void executeTask() override
    {
        advance();
    }

private:
    UpdateState state()
    {
        auto entity = m_entity;
        auto root = entity->rootEntity();
        UpdateState state = root->rootState();
        state.isRoot = !entity->m_parent;
        state.forced = m_forced;
        if(entity->m_parent && !m_parentResolved)
        {
            entity->m_parent->load(Net::Mode::Online);
            state.parentBusy = entity->m_parent->getCurrentTask() != nullptr;
        }
        state.current = entity->isCurrent();
        state.hasLocal = entity->isLoaded();
        state.fetch = m_fetch;
        state.rootRefreshed = m_rootRefreshed;
        return state;
    }

    void advance()
    {
        auto entity = m_entity;
        switch(nextUpdateStep(state()))
        {
            case UpdateStep::Done:
            {
                QMetaObject::invokeMethod(this, "emitSucceeded", Qt::QueuedConnection);
                return;
            }
            case UpdateStep::WaitForParent:
            {
                // whatever happens to the parent, this goes on with the hash it has then
                connect(entity->m_parent->getCurrentTask().get(), &Task::finished, this, [this]()
                {
                    m_parentResolved = true;
                    advance();
                });
                return;
            }
            case UpdateStep::Fetch:
            {
                fetch();
                return;
            }
            case UpdateStep::RefreshRoot:
            {
                qDebug() << "Fetching the meta index again, the hash of" << entity->localFilename() << "didn't match";
                auto root = entity->rootEntity();
                root->startUpdate(true);
                m_rootRefreshed = true;
                m_fetch = UpdateState::Fetch::NotYet;
                // the parent may have to be fetched again too
                m_parentResolved = false;
                auto rootTask = root->getCurrentTask();
                if(!rootTask)
                {
                    advance();
                    return;
                }
                connect(rootTask.get(), &Task::finished, this, [this]() { advance(); });
                return;
            }
            case UpdateStep::UseLocal:
            {
                qWarning() << "Couldn't update" << entity->localFilename() << ", using the local copy:" << m_failReason;
                emitSucceeded();
                return;
            }
            case UpdateStep::Fail:
            {
                emitFailed(m_failReason);
                return;
            }
        }
    }

    void fetch()
    {
        auto entity = m_entity;
        m_job = new NetJob(QObject::tr("Download of meta file %1").arg(entity->localFilename()), APPLICATION->network());
        Net::Download::Ptr dl;
        if(entity->m_sha256.isEmpty())
        {
            // nothing says what it should be (the root, or something the parent doesn't list): ask the server
            auto entry = APPLICATION->metacache()->resolveEntry("meta", entity->localFilename());
            entry->setStale(true);
            dl = Net::Download::makeCached(entity->url(), entry);
        }
        else
        {
            dl = Net::Download::makeFile(entity->url(), QDir("meta").absoluteFilePath(entity->localFilename()));
            dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha256, entity->m_sha256));
        }
        /*
         * The validator parses the file and loads it into the object.
         * If that fails, the file is not written to storage.
         */
        dl->addValidator(new ParsingValidator(entity));
        m_job->addNetAction(dl);
        connect(m_job.get(), &NetJob::progress, this, &Task::setProgress);
        connect(m_job.get(), &NetJob::succeeded, this, [this, entity]()
        {
            m_job.reset();
            entity->m_loadStatus = LoadStatus::Remote;
            if(!entity->m_parent)
            {
                entity->m_fetched.start();
                entity->m_fetchFailed.invalidate();
            }
            emitSucceeded();
        });
        connect(m_job.get(), &NetJob::failed, this, [this, entity, dl](QString reason)
        {
            m_job.reset();
            if(!entity->m_parent)
            {
                entity->m_fetchFailed.start();
            }
            bool refused = dl->networkError() == QNetworkReply::NoError || dl->wasNotFound();
            m_fetch = refused ? UpdateState::Fetch::Refused : UpdateState::Fetch::Failed;
            m_failReason = reason;
            advance();
        });
        m_job->start();
    }

private:
    BaseEntity *m_entity;
    bool m_forced;
    NetJob::Ptr m_job;
    bool m_parentResolved = false;
    bool m_rootRefreshed = false;
    UpdateState::Fetch m_fetch = UpdateState::Fetch::NotYet;
    QString m_failReason;
};
}

Meta::BaseEntity::~BaseEntity()
{
}

QUrl Meta::BaseEntity::url() const
{
    return QUrl(BuildConfig.META_URL).resolved(localFilename());
}

QByteArray Meta::BaseEntity::sha256() const
{
    return m_sha256;
}

void Meta::BaseEntity::setSha256(const QByteArray& sha256)
{
    m_sha256 = sha256;
}

void Meta::BaseEntity::setParentEntity(Meta::BaseEntity* parent)
{
    m_parent = parent;
}

bool Meta::BaseEntity::isCurrent() const
{
    return !m_sha256.isEmpty() && m_sha256 == m_loadedSha256;
}

bool Meta::BaseEntity::parseData(const QByteArray& data, const QString& what)
{
    try
    {
        auto doc = Json::requireDocument(data, what);
        auto obj = Json::requireObject(doc, what);
        parse(obj);
        m_loadedSha256 = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
        return true;
    }
    catch (const Exception &e)
    {
        qWarning() << "Unable to parse" << what << ":" << e.cause();
        return false;
    }
}

bool Meta::BaseEntity::loadLocalFile()
{
    const QString fname = QDir("meta").absoluteFilePath(localFilename());
    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }
    if (parseData(file.readAll(), fname))
    {
        return true;
    }
    // just make sure it's gone and we never consider it again.
    file.close();
    QFile::remove(fname);
    return false;
}

void Meta::BaseEntity::load(Net::Mode loadType)
{
    // load local file if nothing is loaded yet
    if(!isLoaded())
    {
        if(loadLocalFile())
        {
            m_loadStatus = LoadStatus::Local;
        }
    }
    // if we need remote update, run the update task
    if(loadType == Net::Mode::Offline || !shouldStartRemoteUpdate())
    {
        return;
    }
    startUpdate(false);
}

Meta::UpdateState Meta::BaseEntity::rootState() const
{
    UpdateState state;
    state.rootFetchedAgo = m_fetched.isValid() ? m_fetched.elapsed() : -1;
    state.rootFailedAgo = m_fetchFailed.isValid() ? m_fetchFailed.elapsed() : -1;
    state.rootBusy = m_updateStatus == UpdateStatus::InProgress;
    return state;
}

void Meta::BaseEntity::startUpdate(bool force)
{
    if(m_updateStatus == UpdateStatus::InProgress)
    {
        return;
    }
    // only start a task if there's something to do or wait for
    UpdateState state = rootEntity()->rootState();
    state.isRoot = !m_parent;
    state.forced = force;
    if(m_parent)
    {
        m_parent->load(Net::Mode::Online);
        state.parentBusy = m_parent->getCurrentTask() != nullptr;
    }
    state.current = isCurrent();
    state.hasLocal = isLoaded();
    if(nextUpdateStep(state) == UpdateStep::Done)
    {
        return;
    }
    m_updateTask = new UpdateTask(this, force);
    m_updateStatus = UpdateStatus::InProgress;
    QObject::connect(m_updateTask.get(), &Task::succeeded, [&]()
    {
        m_updateStatus = UpdateStatus::Succeeded;
        m_updateTask.reset();
    });
    QObject::connect(m_updateTask.get(), &Task::failed, [&]()
    {
        m_updateStatus = UpdateStatus::Failed;
        m_updateTask.reset();
    });
    m_updateTask->start();
}

Meta::BaseEntity * Meta::BaseEntity::rootEntity()
{
    auto entity = this;
    while(entity->m_parent)
    {
        entity = entity->m_parent;
    }
    return entity;
}

bool Meta::BaseEntity::isLoaded() const
{
    return m_loadStatus > LoadStatus::NotLoaded;
}

bool Meta::BaseEntity::shouldStartRemoteUpdate() const
{
    // TODO: version-locks and offline mode?
    return m_updateStatus != UpdateStatus::InProgress;
}

Task::Ptr Meta::BaseEntity::getCurrentTask()
{
    if(m_updateStatus == UpdateStatus::InProgress)
    {
        return m_updateTask;
    }
    return nullptr;
}
