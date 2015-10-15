#include "WonkoVersion.h"

#include <QDateTime>

#include "tasks/BaseWonkoEntityLocalLoadTask.h"
#include "tasks/BaseWonkoEntityRemoteLoadTask.h"
#include "format/WonkoFormat.h"

WonkoVersion::WonkoVersion(const QString &uid, const QString &version)
	: BaseVersion(), m_uid(uid), m_version(version)
{
}

QString WonkoVersion::descriptor()
{
	return m_version;
}
QString WonkoVersion::name()
{
	return m_version;
}
QString WonkoVersion::typeString() const
{
	return m_type;
}

QDateTime WonkoVersion::time() const
{
	return QDateTime::fromMSecsSinceEpoch(m_time * 1000, Qt::UTC);
}

std::unique_ptr<Task> WonkoVersion::remoteUpdateTask()
{
	return std::make_unique<WonkoVersionRemoteLoadTask>(this, this);
}
std::unique_ptr<Task> WonkoVersion::localUpdateTask()
{
	return std::make_unique<WonkoVersionLocalLoadTask>(this, this);
}

void WonkoVersion::merge(const std::shared_ptr<BaseWonkoEntity> &other)
{
	WonkoVersionPtr version = std::dynamic_pointer_cast<WonkoVersion>(other);
	if (m_type != version->m_type)
	{
		setType(version->m_type);
	}
	if (m_time != version->m_time)
	{
		setTime(version->m_time);
	}
	if (m_requires != version->m_requires)
	{
		setRequires(version->m_requires);
	}

	setData(version->m_mainClass, version->m_appletClass, version->m_assets,
			version->m_minecraftArguments, version->m_tweakers,
			version->m_libraries, version->m_jarMods, version->m_order);
}

QString WonkoVersion::localFilename() const
{
	return m_uid + '/' + m_version + ".json";
}
QJsonObject WonkoVersion::serialized() const
{
	return WonkoFormat::serializeVersion(this);
}

void WonkoVersion::setType(const QString &type)
{
	m_type = type;
	emit typeChanged();
}
void WonkoVersion::setTime(const qint64 time)
{
	m_time = time;
	emit timeChanged();
}
void WonkoVersion::setRequires(const QVector<WonkoReference> &requires)
{
	m_requires = requires;
	emit requiresChanged();
}

void WonkoVersion::setData(const QString &mainClass, const QString &appletClass, const QString &assets,
						   const QString &minecraftArguments, const QStringList &tweakers,
						   const QVector<QJsonObject> &libraries, const QVector<QJsonObject> &jarMods, const int order)
{
	m_mainClass = mainClass;
	m_appletClass = appletClass;
	m_assets = assets;
	m_minecraftArguments = minecraftArguments;
	m_tweakers = tweakers;
	m_libraries = libraries;
	m_jarMods = jarMods;
	m_order = order;
}
