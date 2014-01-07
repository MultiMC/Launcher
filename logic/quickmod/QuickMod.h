#pragma once

#include <QObject>
#include <QMap>
#include <QMetaType>
#include <QUrl>
#include <QStringList>
#include <QIcon>
#include <memory>

class QuickMod;

class QuickMod : public QObject
{
	Q_OBJECT
public:
	explicit QuickMod(QObject *parent = 0);

	enum ModType
	{
		ForgeMod,
		ForgeCoreMod,
		ResourcePack,
		ConfigPack,
		Group
	};
	Q_ENUMS(ModType)

	bool isValid() const
	{
		return !m_name.isEmpty();
	}

	QString uid() const;
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
	ModType type() const
	{
		return m_type;
	}
	QUrl versionsUrl() const
	{
		return m_versionsUrl;
	}
	bool isStub() const
	{
		return m_stub;
	}

	QByteArray hash() const
	{
		return m_hash;
	}
	bool parse(const QByteArray &data, QString *errorMessage = 0);

	bool compare(const QuickMod *other) const;

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
	ModType m_type;
	QUrl m_versionsUrl;
	bool m_stub;

	QByteArray m_hash;

	bool m_loadingIcon;
	bool m_loadingLogo;

	void fetchImages();
	QString fileName(const QUrl &url) const;
};
Q_DECLARE_METATYPE(QuickMod *)
