// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "tasks/Task.h"
#include <QFutureWatcher>
#include <functional>
class QWidget;
class MinecraftInstance;

// Disk work runs off the GUI thread. Commit/restore are deliberately non-cancellable.
class PackUpdateWork : public Task
{
    Q_OBJECT
public:
    PackUpdateWork(const QString &label, std::function<void()> work);
    ~PackUpdateWork() override { m_watcher.waitForFinished(); }
protected:
    void executeTask() override;
private:
    std::function<void()> m_work;
    QFutureWatcher<QString> m_watcher;
};
namespace PackUpdateDialog {
void run(QWidget *parent, MinecraftInstance *instance, bool restoreOnly = false);
}
