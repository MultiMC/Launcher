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

#pragma once

#include <QObject>
#include <QVariant>
#include <QSharedPointer>

/*!
 * List of available Mojang accounts.
 * This should be loaded in the background by MultiMC on startup.
 */
class RuntimeManager : public QObject
{
    Q_OBJECT
public:
    explicit RuntimeManager(QObject *parent = 0);
    virtual ~RuntimeManager() noexcept;

    const RuntimePtr at(int i) const;
    int count() const;

    void addRuntime(const RuntimePtr runtime);
    void removeRuntime(QString runtimeId);

    // requesting a refresh pushes it to the front of the queue
    void requestRefresh(QString runtimeId);
    // queuing a refresh will let it go to the back of the queue (unless it's somewhere inside the queue already)
    void queueRefresh(QString runtimeId);

    /*!
     * Sets the path to load/save the list file from/to.
     * If autosave is true, this list will automatically save to the given path whenever it changes.
     * THIS FUNCTION DOES NOT LOAD THE LIST. If you set autosave, be sure to call loadList() immediately
     * after calling this function to ensure an autosaved change doesn't overwrite the list you intended
     * to load.
     */
    void setListFilePath(QString path, bool autosave = false);

    bool loadLocal();
    bool saveLocal();
    bool isActive() const;

protected:
    void beginActivity();
    void endActivity();

private:
    const char* m_name;
    uint32_t m_activityCount = 0;
signals:
    void listChanged();
    void listActivityChanged();
    void defaultAccountChanged();
    void activityChanged(bool active);

public slots:
    void runtimeChanged();
    void runtimeActivityChanged(bool active);
    void fillQueue();

private slots:
    void tryNext();

    void taskSucceeded();
    void taskFailed(QString reason);

protected:
    QList<QString> m_refreshQueue;
    QTimer *m_refreshTimer;
    QTimer *m_nextTimer;
    shared_qobject_ptr<AccountTask> m_currentTask;

    /*!
     * Called whenever the list changes.
     * This emits the listChanged() signal and autosaves the list (if autosave is enabled).
     */
    void onListChanged();

    QList<RuntimePtr> m_data;

    //! Path to the account list file. Empty string if there isn't one.
    QString m_listFilePath;

    /*!
     * If true, the account list will automatically save to the account list path when it changes.
     * Ignored if m_listFilePath is blank.
     */
    bool m_autosave = false;
};

