#pragma once

#include <QAbstractListModel>
#include <memory>

#include "BaseWonkoEntity.h"

#include "multimc_logic_export.h"

class Task;
using WonkoVersionListPtr = std::shared_ptr<class WonkoVersionList>;

class MULTIMC_LOGIC_EXPORT WonkoIndex : public QAbstractListModel, public BaseWonkoEntity
{
	Q_OBJECT
public:
	explicit WonkoIndex(QObject *parent = nullptr);
	explicit WonkoIndex(const QVector<WonkoVersionListPtr> &lists, QObject *parent = nullptr);

	enum
	{
		UidRole = Qt::UserRole,
		NameRole,
		ListPtrRole
	};

	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	Task *remoteUpdateTask() override;
	Task *localUpdateTask() override;

	QString localFilename() const override { return "index.json"; }
	QJsonObject serialized() const override;

	// queries
	bool hasUid(const QString &uid) const;
	WonkoVersionListPtr getList(const QString &uid) const;

	QVector<WonkoVersionListPtr> lists() const { return m_lists; }

public: // for usage by parsers only
	void merge(const BaseWonkoEntity::Ptr &other);

private:
	QVector<WonkoVersionListPtr> m_lists;
	QHash<QString, WonkoVersionListPtr> m_uids;

	void connectVersionList(const int row, const WonkoVersionListPtr &list);
};
