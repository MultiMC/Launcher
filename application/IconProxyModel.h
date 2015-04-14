// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QIdentityProxyModel>

class IconProxyModel : public QIdentityProxyModel
{
	Q_OBJECT
public:
	explicit IconProxyModel(QObject *parent = nullptr);

	QVariant data(const QModelIndex &proxyIndex, int role) const override;

	static IconProxyModel *mixin(QAbstractItemModel *source);

private slots:
	void resetSourceModel();
};
