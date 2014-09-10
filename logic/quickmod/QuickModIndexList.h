#pragma once

#include <QAbstractItemModel>
#include <memory>

class QUrl;
class QString;
class SettingsObject;
class QuickModRef;
typedef std::shared_ptr<class QuickModMetadata> QuickModMetadataPtr;

// TODO redownload index

/**
 * @brief The QuickModIndexList class is a model for indexes, as well as an interface to the
 * Indices property in quickmod.cfg
 */
class QuickModIndexList : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit QuickModIndexList(QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;

	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	QModelIndex index(int row, int column, const QModelIndex &parent) const override;
	QModelIndex parent(const QModelIndex &child) const override;

	QUrl repositoryIndexUrl(const QString &repository) const;
	bool haveRepositoryIndexUrl(const QString &repository) const;

	void setRepositoryIndexUrl(const QString &repository, const QUrl &url);
	QList<QUrl> indices() const;

	bool haveUid(const QuickModRef &uid, const QString &repo) const;

public slots:
	void reload();

private:
	struct Repo
	{
		explicit Repo(const QString &name, const QString &url = QString())
			: name(name), url(url)
		{
		}
		explicit Repo() {}

		QString name;
		QString url;
		QList<QuickModMetadataPtr> mods;
	};
	QList<Repo> m_repos;
};
