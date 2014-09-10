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

#include "MMCError.h"
#include "QuickModRef.h"

typedef std::shared_ptr<class QuickModMetadata> QuickModMetadataPtr;

class QuickModMetadata : public QObject
{
	Q_OBJECT
public:
	explicit QuickModMetadata(QObject *parent = 0);
	~QuickModMetadata();

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
		Logo,
		Invalid
	};

	QuickModRef uid() const
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

	QStringList authorTypes() const
	{
		return m_authors.keys();
	}
	QStringList authors(const QString &type) const
	{
		return m_authors.value(type);
	}

	QUrl safeUrl(const UrlType type) const
	{
		const auto urls = url(type);
		return urls.isEmpty() ? QUrl() : urls.first();
	}
	QList<QUrl> url(const UrlType type) const
	{
		return m_urls[urlId(type)];
	}
	QUrl websiteUrl() const
	{
		return safeUrl(Website);
	}

	QUrl iconUrl() const
	{
		return safeUrl(Icon);
	}
	QIcon icon();

	QUrl logoUrl() const
	{
		return safeUrl(Logo);
	}
	QPixmap logo();

	QUrl updateUrl() const
	{
		return m_updateUrl;
	}

	QMap<QuickModRef, QUrl> references() const
	{
		return m_references;
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

	void parse(const QJsonObject &mod);
	QJsonObject toJson() const;

	bool compare(const QuickModMetadataPtr other) const;

	static UrlType urlType(const QString &id);
	static QString urlId(const UrlType type);
	static QString humanUrlId(const UrlType type);
	static QList<UrlType> urlTypes();

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
	friend class QuickModBuilder;
	friend class TestsInternal;
	QuickModRef m_uid;
	QString m_repo;
	QString m_name;
	QString m_description;
	QMap<QString, QList<QUrl>> m_urls;
	QMap<QString, QStringList> m_authors;
	QIcon m_icon;
	QPixmap m_logo;
	QUrl m_updateUrl;
	QMap<QuickModRef, QUrl> m_references;
	QString m_modId;
	QStringList m_categories;
	QStringList m_tags;
	QString m_license;

	bool m_imagesLoaded;

	void fetchImages();
	QString fileName(const QUrl &url) const;
};

Q_DECLARE_METATYPE(QuickModMetadataPtr)
Q_DECLARE_METATYPE(QuickModRef)
