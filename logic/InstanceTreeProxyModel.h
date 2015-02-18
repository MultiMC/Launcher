#pragma once

#include <QAbstractProxyModel>

class InstanceTreeProxyModel : public QAbstractProxyModel
{
	Q_OBJECT
public:
	explicit InstanceTreeProxyModel(QObject *parent = nullptr);

	void setSourceModel(QAbstractItemModel *sourceModel) override;

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &child = QModelIndex()) const override;

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;

	QVariant data(const QModelIndex &proxyIndex, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
	QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;

private:
	//   proxy                  source
	QMap<QPersistentModelIndex, QPersistentModelIndex> m_proxyToSource;
	//   child (proxy)          parent (proxy)
	QMap<QPersistentModelIndex, QPersistentModelIndex> m_proxyInstanceToGroup;
	//   group (proxy)
	QMap<QPersistentModelIndex, QString> m_proxyGroupToName;
	QMap<QString, int> m_groupIds;

	void setupGroups();
};
