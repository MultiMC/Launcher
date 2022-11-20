#pragma once

#include "tasks/Task.h"
#include "net/NetJob.h"
#include "PackManifest.h"

namespace CurseForge
{
class FileResolvingTask : public Task
{
    Q_OBJECT
public:
    explicit FileResolvingTask(shared_qobject_ptr<QNetworkAccessManager> network, CurseForge::Manifest &toProcess);
    virtual ~FileResolvingTask() {};

    const CurseForge::Manifest &getResults() const
    {
        return m_toProcess;
    }

protected:
    virtual void executeTask() override;

protected slots:
    void netJobFinished();

private: /* data */
    shared_qobject_ptr<QNetworkAccessManager> m_network;
    CurseForge::Manifest m_toProcess;
    QVector<QByteArray> results;
    NetJob::Ptr m_dljob;
};
}
