#include "TwitchVersionList.h"
#include "TwitchData.h"

TwitchVersionList::TwitchVersionList(const Twitch::Addon &addon, QObject *parent) : 
    BaseVersionList(parent),
    m_addon(addon)
{

}

shared_qobject_ptr<Task> TwitchVersionList::getLoadTask()
{
    load();
    return getCurrentTask();
}

bool TwitchVersionList::isLoaded()
{
    return m_status == TwitchVersionList::Status::Done;
}

const BaseVersionPtr TwitchVersionList::at(int i) const
{
    return m_vlist.at(i);
}

int TwitchVersionList::count() const
{
    return m_vlist.count();
}

bool sortPackVersions(BaseVersionPtr left, BaseVersionPtr right)
{
    auto rleft = std::dynamic_pointer_cast<TwitchVersion>(left);
    auto rright = std::dynamic_pointer_cast<TwitchVersion>(right);
    return (*rleft) > (*rright);
}

void TwitchVersionList::sortVersions()
{
    beginResetModel();
    std::sort(m_vlist.begin(), m_vlist.end(), sortPackVersions);
    endResetModel();
}

QVariant TwitchVersionList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() > count())
        return QVariant();

    auto version = std::dynamic_pointer_cast<TwitchVersion>(m_vlist[index.row()]);
    switch (role)
    {
        case VersionPointerRole:
            return qVariantFromValue(m_vlist[index.row()]);
        case VersionIdRole:
            return version->descriptor();
        case VersionRole:
            return version->name();
        case RecommendedRole:
            return version->recommended;
        default:
            return QVariant();
    }
}

BaseVersionList::RoleList TwitchVersionList::providesRoles() const
{
    return {VersionPointerRole, VersionIdRole, VersionRole, RecommendedRole};
}


void TwitchVersionList::updateListData(QList<BaseVersionPtr> versions)
{
    beginResetModel();
    m_vlist = versions;
    sortVersions();
    if(m_vlist.size())
    {
        auto best = std::dynamic_pointer_cast<TwitchVersion>(m_vlist[0]);
        best->recommended = true;
    }
    endResetModel();
    m_status = Status::Done;
    m_loadTask.reset();
}

void TwitchVersionList::load()
{
    if(m_status != Status::InProgress)
    {
        m_status = Status::InProgress;
        m_loadTask = new TwitchVersionLoadTask(this);
        m_loadTask->start();
    }
}

shared_qobject_ptr<Task> TwitchVersionList::getCurrentTask()
{
    if(m_status == Status::InProgress)
    {
        return m_loadTask;
    }
    return nullptr;
}

TwitchVersionLoadTask::TwitchVersionLoadTask(TwitchVersionList *vlist)
{
    addon = vlist;
}

TwitchVersionLoadTask::~TwitchVersionLoadTask()
{

}

void TwitchVersionLoadTask::executeTask() 
{
    auto addon_files = addon->m_addon.files;
    QList<TwitchVersionPtr> version_list;

    for (auto file : addon_files)
    {
        TwitchVersionPtr version(new TwitchVersion(file));
        version_list.append(version);
    }

    QList<BaseVersionPtr> twitch_bvp;
    for (auto file : version_list)
    {
        BaseVersionPtr bp_twitch = std::dynamic_pointer_cast<BaseVersion>(file);

        if (bp_twitch)
        {
            twitch_bvp.append(bp_twitch);
        }
    }

    addon->updateListData(twitch_bvp);
    emitSucceeded();
}
