#pragma once

#include "BaseVersion.h"

#include <QMap>
#include <QDateTime>

class WonkoPackageVersion;
using WonkoVersionPtr = std::shared_ptr<WonkoPackageVersion>;
using ResourcePtr = std::shared_ptr<class BaseResource>;

class WonkoPackageVersion : public BaseVersion
{
public:
	virtual ~WonkoPackageVersion()
	{
	}

	void load(const QJsonObject &obj, const QString &uid = QString());

	QString uid() const
	{
		return m_uid;
	}
	virtual QString descriptor() override
	{
		return m_id;
	}
	virtual QString name()
	{
		return m_id;
	}
	virtual QString typeString() const
	{
		return m_type;
	}
	virtual QDateTime timestamp() const
	{
		return m_time;
	}

	QMap<QString, QString> dependencies() const
	{
		return m_dependencies;
	}

	QMap<QString, ResourcePtr> resources() const
	{
		return m_resources;
	}

	template <typename T> std::shared_ptr<T> resource(const QString &key) const
	{
		return std::dynamic_pointer_cast<T>(m_resources.value(key));
	}

private:
	QString m_uid;
	QString m_id;
	QString m_type;
	QDateTime m_time;
	QMap<QString, QString> m_dependencies;
	QMap<QString, ResourcePtr> m_resources;
};
