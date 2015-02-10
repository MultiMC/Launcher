#pragma once

#include <memory>
#include <QString>
#include <QStringList>
#include <QMap>

#include "Json.h"

class QJsonValue;
class Task;
using ResourcePtr = std::shared_ptr<class BaseResource>;

class BaseResource
{
public:
	virtual ~BaseResource()
	{
	}

	virtual void clear() {}
	virtual Task *updateTask() const
	{
		return nullptr;
	}
	virtual Task *prelaunchTask() const
	{
		return nullptr;
	}

	virtual void applyTo(const ResourcePtr &target) const
	{
	}
};

class StringResource : public BaseResource
{
public:
	explicit StringResource(const QString &data) : m_data(data) {}
	QString data() const
	{
		return m_data;
	}
	void applyTo(const ResourcePtr &target) const override
	{
		std::dynamic_pointer_cast<StringResource>(target)->m_data = m_data;
	}

private:
	QString m_data;
};
class StringListResource : public BaseResource
{
public:
	explicit StringListResource(const QStringList &data) : m_data(data) {}
	QStringList data() const
	{
		return m_data;
	}
	void applyTo(const ResourcePtr &target) const override
	{
		std::dynamic_pointer_cast<StringListResource>(target)->m_data = m_data;
	}

private:
	QStringList m_data;
};

class FoldersResource : public BaseResource
{
public:
	explicit FoldersResource(const QMap<QString, QStringList> &folders) : m_folders(folders) {}

	QStringList folderPaths() const
	{
		return m_folders.keys();
	}
	QStringList folderPathTypes(const QString &path) const
	{
		return m_folders[path];
	}

	void applyTo(const ResourcePtr &target) const override
	{
		std::dynamic_pointer_cast<FoldersResource>(target)->m_folders.unite(m_folders);
	}

private:
	QMap<QString, QStringList> m_folders;
};

class BaseResourceFactory
{
public:
	virtual ~BaseResourceFactory() {}

	virtual bool supportsFormatVersion(const int version) const = 0;
	virtual QStringList keys(const int formatVersion) const = 0;
	virtual ResourcePtr create(const int formatVersion, const QString &key, const QJsonValue &data) const = 0;
};
class StandardResourceFactory : public BaseResourceFactory
{
protected:
	const int m_version;
	const QString m_key;

	explicit StandardResourceFactory(const int version, const QString &key)
		: m_version(version), m_key(key) {}

public:
	bool supportsFormatVersion(const int version) const override { return m_version == version; }
	QStringList keys(const int formatVersion) const override { return {m_key}; }
};

class StringResourceFactory : public StandardResourceFactory
{
public:
	explicit StringResourceFactory(const int version, const QString &key) : StandardResourceFactory(version, key) {}

	ResourcePtr create(const int formatVersion, const QString &key, const QJsonValue &data) const override;
};
class StringListResourceFactory : public StandardResourceFactory
{
public:
	explicit StringListResourceFactory(const int version, const QString &key) : StandardResourceFactory(version, key) {}

	ResourcePtr create(const int formatVersion, const QString &key, const QJsonValue &data) const override;
};
class FoldersResourceFactory : public BaseResourceFactory
{
public:
	bool supportsFormatVersion(const int version) const override { return 0 == version; }
	QStringList keys(const int formatVersion) const override { return {"general.folders"}; }
	ResourcePtr create(const int formatVersion, const QString &key, const QJsonValue &data) const override;
};
