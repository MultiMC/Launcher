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
	QuickModParseError(QString cause) : MMCError(cause) {}
	virtual ~QuickModParseError() noexcept {}
};

class QuickMod : public QObject
{
	Q_OBJECT
public:
	explicit QuickMod(QObject *parent = 0);

	bool isValid() const
	{
		return !m_name.isEmpty();
	}

	QString uid() const
	{
		return m_uid;
	}
	QString name() const
	{
		return m_name;
	}
	QString description() const
	{
		return m_description;
	}
	QUrl websiteUrl() const
	{
		return m_websiteUrl;
	}
	QUrl verifyUrl() const
	{
		return m_verifyUrl;
	}
	QUrl iconUrl() const
	{
		return m_iconUrl;
	}
	QIcon icon();
	QUrl logoUrl() const
	{
		return m_logoUrl;
	}
	QPixmap logo();
	QUrl updateUrl() const
	{
		return m_updateUrl;
	}
	QMap<QString, QUrl> references() const
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
	QUrl versionsUrl() const
	{
		return m_versionsUrl;
	}
	QList<QuickModVersionPtr> versions() const
	{
		return m_versions;
	}
	void setVersions(const QList<QuickModVersionPtr> &versions);
	QuickModVersionPtr version(const QString &name) const;
	QuickModVersionPtr latestVersion(const QString &mcVersion) const;

	QByteArray hash() const
	{
		return m_hash;
	}
	void parse(const QByteArray &data);

	bool compare(const QuickModPtr other) const;

signals:
	void iconUpdated();
	void logoUpdated();
	void versionsUpdated();

private
slots:
	void iconDownloadFinished(int index);
	void logoDownloadFinished(int index);

private:
	friend class QuickModFilesUpdater;
	friend class QuickModTest;
	friend class QuickModsListTest;
	friend class TestsInternal;
	QString m_uid;
	QString m_name;
	QString m_description;
	QUrl m_websiteUrl;
	QUrl m_verifyUrl;
	QUrl m_iconUrl;
	QIcon m_icon;
	QUrl m_logoUrl;
	QPixmap m_logo;
	QUrl m_updateUrl;
	QMap<QString, QUrl> m_references;
	QString m_nemName;
	QString m_modId;
	QStringList m_categories;
	QStringList m_tags;
	QUrl m_versionsUrl;

	QList<QuickModVersionPtr> m_versions;

	QByteArray m_hash;

	bool m_imagesLoaded;

	void fetchImages();
	QString fileName(const QUrl &url) const;
};
Q_DECLARE_METATYPE(QuickModPtr)

