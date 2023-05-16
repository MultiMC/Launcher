#include "SequentialTask.h"

SequentialTask::SequentialTask(QObject *parent) : Task(parent), m_currentIndex(-1)
{
}

void SequentialTask::addTask(Task::Ptr task)
{
    m_queue.append(task);
}

void SequentialTask::executeTask()
{
    m_currentIndex = -1;
    startNext();
}

void SequentialTask::startNext()
{
    if (m_currentIndex != -1)
    {
        Task::Ptr previous = m_queue[m_currentIndex];
        disconnect(previous.get(), 0, this, 0);
    }
    m_currentIndex++;
    if (m_queue.isEmpty() || m_currentIndex >= m_queue.size())
    {
        emitSucceeded();
        return;
    }
    Task::Ptr next = m_queue[m_currentIndex];
    connect(next.get(), &Task::failed, this, &SequentialTask::subTaskFailed);
    connect(next.get(), &Task::status, this, &SequentialTask::subTaskStatus);
    connect(next.get(), &Task::progress, this, &SequentialTask::subTaskProgress);
    connect(next.get(), &Task::succeeded, this, &SequentialTask::startNext);
    next->start();
}

void SequentialTask::subTaskFailed(const QString &msg)
{
    emitFailed(msg);
}
void SequentialTask::subTaskStatus(const QString &msg)
{
    setStatus(msg);
}
void SequentialTask::subTaskProgress(qint64 current, qint64 total)
{
    if(total == 0)
    {
        setProgress(0, 100);
        return;
    }
    setProgress(current, total);
}
