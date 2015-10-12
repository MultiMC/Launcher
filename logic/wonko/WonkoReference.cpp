#include "WonkoReference.h"

WonkoReference::WonkoReference(const QString &uid)
	: m_uid(uid)
{
}

QString WonkoReference::uid() const
{
	return m_uid;
}

QString WonkoReference::version() const
{
	return m_version;
}
void WonkoReference::setVersion(const QString &version)
{
	m_version = version;
}

bool WonkoReference::operator==(const WonkoReference &other) const
{
	return m_uid == other.m_uid && m_version == other.m_version;
}
bool WonkoReference::operator!=(const WonkoReference &other) const
{
	return m_uid != other.m_uid || m_version != other.m_version;
}
