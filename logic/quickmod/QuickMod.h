/* Copyright 2013 MultiMC Contributors
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
#include <QMap>
#include <QMetaType>
#include <QUrl>
#include <QStringList>
#include <QIcon>
#include <memory>
#include <MMCError.h>

class QuickMod;
class QuickModVersion;

typedef std::shared_ptr<QuickMod> QuickModPtr;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;

class QuickModParseError : public MMCError
{
public:
	QuickModParseError(QString cause) : MMCError(cause)
	{
	}
	virtual ~QuickModParseError() noexcept
	{
	}
};

class QuickModUid
{
public:
	QuickModUid();
	explicit QuickModUid(const QString &uid);

	QString toString() const;
	QuickModPtr mod() const;
	QList<QuickModPtr> mods() const;

	bool isValid() const
	{
		return !m_uid.isEmpty();
	}
	bool operator==(const QuickModUid &other) const
	{
		return m_uid == other.m_uid;
	}
	bool operator==(const QString &other) const
	{
		return m_uid == other;
	}
	bool operator<(const QuickModUid &other) const
	{
		return m_uid < other.m_uid;
	}

private:
	QString m_uid;
};
QDebug operator<<(QDebug &dbg, const QuickModUid &uid);
uint qHash(const QuickModUid &uid);

class QuickMod : public QObject
{
	Q_OBJECT
public:
	explicit QuickMod(QObject *parent = 0);
	~QuickMod();

	bool isValid() const
	{
		return !m_name.isEmpty();
	}

	enum UrlType
	{
		Website,
		Wiki,
		Forum,
		Donation,
		Issues,
		Source,
		Icon,
		Logo
	};

	QuickModUid uid() const
	{
		return m_uid;
	}
	QString internalUid() const
	{
		return m_repo + '.' + m_uid.toString();
	}
	QString repo() const
	{
		return m_repo;
	}
	QString name() const
	{
		return m_name;
	}
	QString description() const
	{
		return m_description;
	}
	QList<QUrl> url(const UrlType type) const;
	QUrl websiteUrl() const
	{
		auto websites = url(Website);
		return websites.isEmpty() ? QUrl() : websites.first();
	}
	QUrl verifyUrl() const
	{
		return m_verifyUrl;
	}
	QUrl iconUrl() const
	{
		auto websites = url(Icon);
		return websites.isEmpty() ? QUrl() : websites.first();
	}
	QIcon icon();
	QUrl logoUrl() const
	{
		auto websites = url(Logo);
		return websites.isEmpty() ? QUrl() : websites.first();
	}
	QPixmap logo();
	QUrl updateUrl() const
	{
		return m_updateUrl;
	}
	QMap<QuickModUid, QUrl> references() const
	{
		return m_references;
	}
	QString nemName() const
	{
		return m_nemName;
	}
	QString modId() const
	{
		return m_modId;
	}
	QStringList categories() const
	{
		return m_categories;
	}
	QStringList tags() const
	{
		return m_tags;
	}
	QString license() const
	{
		return m_license;
	}
	QList<QuickModVersionPtr> versions() const
	{
		return m_versions;
	}
	/// List of Minecraft versions this QuickMod is compatible with.
	QStringList mcVersions();
	QuickModVersionPtr version(const QString &name) const;
	QuickModVersionPtr latestVersion(const QString &mcVersion) const;
	void sortVersions();

	QByteArray hash() const
	{
		return m_hash;
	}
	void parse(QuickModPtr _this, const QByteArray &data);

	bool compare(const QuickModPtr other) const;

signals:
	void iconUpdated();
	void logoUpdated();

private
slots:
	void iconDownloadFinished(int index);
	void logoDownloadFinished(int index);

private:
	friend class QuickModFilesUpdater;
	friend class QuickModTest;
	friend class QuickModsListTest;
	friend class TestsInternal;
	QuickModUid m_uid;
	QString m_repo;
	QString m_name;
	QString m_description;
	QMap<QString, QList<QUrl>> m_urls;
	QUrl m_verifyUrl;
	QIcon m_icon;
	QPixmap m_logo;
	QUrl m_updateUrl;
	QMap<QuickModUid, QUrl> m_references;
	QStringList m_mcVersionListCache;
	QString m_nemName;
	QString m_modId;
	QStringList m_categories;
	QStringList m_tags;
	QString m_license;

	QList<QuickModVersionPtr> m_versions;

	QByteArray m_hash;

	bool m_imagesLoaded;

	void fetchImages();
	QString fileName(const QUrl &url) const;
};

Q_DECLARE_METATYPE(QuickModPtr)
Q_DECLARE_METATYPE(QuickModUid)
