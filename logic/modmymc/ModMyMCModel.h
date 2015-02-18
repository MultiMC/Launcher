#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <memory>

class ModMyMCModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit ModMyMCModel(QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;

public slots:
	void update();

private:
	struct Entry
	{
		QDateTime timestamp;
		QString mod;
		QString changelog;
		enum Type
		{
			NewMod,
			UpdatedMod
		} type;
	};
	QList<Entry> m_entries;
	Entry createEntry(const QString &data, const QDateTime &timestamp);

	std::shared_ptr<class CacheDownload> m_currentDownload;
};
