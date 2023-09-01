/*
 * Copyright 2023 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QObject>
#include <QObjectPtr.h>
#include <tasks/Task.h>

class JREClient: public QObject {
protected:
    Q_OBJECT

public:
    JREClient(QObject * parent);
    virtual ~JREClient() = default;

    using Ptr = shared_qobject_ptr<JREClient>;

    void refreshManifest();

signals:
    void busyChanged(bool busy);

private:
    Task::Ptr m_manifestTask;
    Task::Ptr m_runtimeTask;
};
