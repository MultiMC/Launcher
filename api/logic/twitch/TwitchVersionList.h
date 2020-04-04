#pragma once

#include <QObject>
#include <QAbstractListModel>

#include "BaseVersionList.h"
#include "TwitchVersion.h"
#include "tasks/Task.h"

#include "QObjectPtr.h"

#include "multimc_logic_export.h"


class TwitchVersionLoadTask;

class MULTIMC_LOGIC_EXPORT TwitchVersionList : public BaseVersionList
{
    Q_OBJECT
    enum class Status
    {
        NotDone,
        InProgress,
        Done
    };
public:
    explicit TwitchVersionList(const Twitch::Addon &addon, QObject *parent = 0);

    shared_qobject_ptr<Task> getLoadTask() override;
    bool isLoaded() override;
    const BaseVersionPtr at(int i) const override;
    int count() const override;
    void sortVersions() override;

    QVariant data(const QModelIndex &index, int role) const override;
    RoleList providesRoles() const override;

public slots:
    void updateListData(QList<BaseVersionPtr> versions) override;

protected:
    void load();
    shared_qobject_ptr<Task> getCurrentTask();

public:
    Twitch::Addon m_addon;

protected:
    Status m_status = Status::NotDone;
    shared_qobject_ptr<TwitchVersionLoadTask> m_loadTask;
    QList<BaseVersionPtr> m_vlist;
};

class TwitchVersionLoadTask : public Task
{
    Q_OBJECT

public:
    explicit TwitchVersionLoadTask(TwitchVersionList *vlist);
    virtual ~TwitchVersionLoadTask();

    void executeTask() override;

protected:
    TwitchVersionList *addon;
};

typedef std::shared_ptr<TwitchVersionList> TwitchVersionListPtr;