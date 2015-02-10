#include "BaseResource.h"

#include "Json.h"

ResourcePtr StringResourceFactory::create(const int formatVersion, const QString &key, const QJsonValue &data) const
{
	Q_ASSERT(formatVersion == m_version);
	Q_ASSERT(key == m_key);
	return std::make_shared<StringResource>(Json::ensureString(data));
}
ResourcePtr StringListResourceFactory::create(const int formatVersion, const QString &key, const QJsonValue &data) const
{
	Q_ASSERT(formatVersion == m_version);
	Q_ASSERT(key == m_key);
	return std::make_shared<StringListResource>(Json::ensureIsArrayOf<QString>(data));
}

ResourcePtr FoldersResourceFactory::create(const int formatVersion, const QString &key, const QJsonValue &data) const
{
	QMap<QString, QStringList> folders;
	const QJsonObject obj = Json::ensureObject(data);
	for (const QString &key : obj.keys())
	{
		folders[key] = Json::ensureIsArrayOf<QString>(obj, key);
	}
	return std::make_shared<FoldersResource>(folders);
}
