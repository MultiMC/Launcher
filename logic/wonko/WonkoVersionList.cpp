#include "WonkoVersionList.h"

#include <QDateTime>

#include "WonkoVersion.h"
#include "tasks/BaseWonkoEntityRemoteLoadTask.h"
#include "tasks/BaseWonkoEntityLocalLoadTask.h"
#include "format/WonkoFormat.h"
#include "WonkoReference.h"

WonkoVersionList::WonkoVersionList(const QString &uid, QObject *parent)
	: BaseVersionList(parent), m_uid(uid)
{
}

Task *WonkoVersionList::getLoadTask()
{
	return remoteUpdateTask();
}

bool WonkoVersionList::isLoaded()
{
	return isLocalLoaded() && isRemoteLoaded();
}

const BaseVersionPtr WonkoVersionList::at(int i) const
{
	return m_versions.at(i);
}
int WonkoVersionList::count() const
{
	return m_versions.size();
}

void WonkoVersionList::sortVersions()
{
	std::sort(m_versions.begin(), m_versions.end(), [](const WonkoVersionPtr &a, const WonkoVersionPtr &b)
	{
		return *a.get() < *b.get();
	});
}

QVariant WonkoVersionList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= m_versions.size() || index.parent().isValid())
	{
		return QVariant();
	}

	WonkoVersionPtr version = m_versions.at(index.row());

	switch (role)
	{
	case VersionPointerRole: return QVariant::fromValue(std::dynamic_pointer_cast<BaseVersion>(version));
	case VersionRole:
	case VersionIdRole:
		return version->version();
	case ParentGameVersionRole:
	{
		const auto it = std::find_if(version->requires().begin(), version->requires().end(),
									 [](const WonkoReference &ref) { return ref.uid() == "net.minecraft"; });
		if (it != version->requires().end())
		{
			return (*it).version();
		}
		return QVariant();
	}
	case TypeRole: return version->type();

	case UidRole: return version->uid();
	case TimeRole: return version->time();
	case RequiresRole: return QVariant::fromValue(version->requires());
	case SortRole: return version->rawTime();
	case WonkoVersionPtrRole: return QVariant::fromValue(version);
	default: return QVariant();
	}
}

BaseVersionList::RoleList WonkoVersionList::providesRoles() const
{
	return {VersionPointerRole, VersionRole, VersionIdRole, ParentGameVersionRole,
				TypeRole, UidRole, TimeRole, RequiresRole, SortRole};
}

QHash<int, QByteArray> WonkoVersionList::roleNames() const
{
	QHash<int, QByteArray> roles = BaseVersionList::roleNames();
	roles.insert(UidRole, "uid");
	roles.insert(TimeRole, "time");
	roles.insert(SortRole, "sort");
	roles.insert(RequiresRole, "requires");
	return roles;
}

Task *WonkoVersionList::remoteUpdateTask()
{
	return new WonkoVersionListRemoteLoadTask(this, this);
}
Task *WonkoVersionList::localUpdateTask()
{
	return new WonkoVersionListLocalLoadTask(this, this);
}

QString WonkoVersionList::localFilename() const
{
	return m_uid + ".json";
}
QJsonObject WonkoVersionList::serialized() const
{
	return WonkoFormat::serializeVersionList(this);
}

QString WonkoVersionList::humanReadable() const
{
	return m_name.isEmpty() ? m_uid : m_name;
}

bool WonkoVersionList::hasVersion(const QString &version) const
{
	return m_lookup.contains(version);
}
WonkoVersionPtr WonkoVersionList::version(const QString &version) const
{
	return m_lookup.value(version);
}

void WonkoVersionList::setName(const QString &name)
{
	m_name = name;
	emit nameChanged(name);
}
void WonkoVersionList::setVersions(const QVector<WonkoVersionPtr> &versions)
{
	beginResetModel();
	m_versions = versions;
	endResetModel();
}

void WonkoVersionList::merge(const BaseWonkoEntity::Ptr &other)
{
	const WonkoVersionListPtr list = std::dynamic_pointer_cast<WonkoVersionList>(other);
	if (m_name != list->m_name)
	{
		setName(list->m_name);
	}

	if (m_versions.isEmpty())
	{
		beginResetModel();
		m_versions = list->m_versions;
		for (int i = 0; i < m_versions.size(); ++i)
		{
			m_lookup.insert(m_versions.at(i)->version(), m_versions.at(i));
			connectVersion(i, m_versions.at(i));
		}
		endResetModel();
	}
	else
	{
		for (const WonkoVersionPtr &version : list->m_versions)
		{
			if (m_lookup.contains(version->version()))
			{
				m_lookup.value(version->version())->merge(version);
			}
			else
			{
				beginInsertRows(QModelIndex(), m_versions.size(), m_versions.size());
				connectVersion(m_versions.size(), version);
				m_versions.append(version);
				m_lookup.insert(version->uid(), version);
				endInsertRows();
			}
		}
	}
}

void WonkoVersionList::connectVersion(const int row, const WonkoVersionPtr &version)
{
	connect(version.get(), &WonkoVersion::requiresChanged, this, [this, row]() { emit dataChanged(index(row), index(row), QVector<int>() << RequiresRole); });
	connect(version.get(), &WonkoVersion::timeChanged, this, [this, row]() { emit dataChanged(index(row), index(row), QVector<int>() << TimeRole << SortRole); });
	connect(version.get(), &WonkoVersion::typeChanged, this, [this, row]() { emit dataChanged(index(row), index(row), QVector<int>() << TypeRole); });
}
