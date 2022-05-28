/*
 * Copyright 2022 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include "InstanceTask.h"
#include "Model.h"

#include <QFuture>
#include <QFutureWatcher>

namespace ImportFTB {

class PackInstallTask : public InstanceTask
{
    Q_OBJECT

public:
    explicit PackInstallTask(Modpack pack);
    virtual ~PackInstallTask() = default;

    bool canAbort() const override;
    bool abort() override;

protected:
    virtual void executeTask() override;

private:
    void install();

private slots:
    void copyFinished();
    void copyAborted();

private:
    bool abortable = false;
    Modpack m_pack;
    QFuture<bool> m_copyFuture;
    QFutureWatcher<bool> m_copyFutureWatcher;
};

}
