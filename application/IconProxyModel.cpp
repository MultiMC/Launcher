// Licensed under the Apache-2.0 license. See README.md for details.

#include "IconProxyModel.h"

#include <QFileInfo>
#include <QDebug>

#include "IconRegistry.h"
#include "MultiMC.h"

IconProxyModel::IconProxyModel(QObject *parent)
	: QIdentityProxyModel(parent)
{
}

QVariant IconProxyModel::data(const QModelIndex &proxyIndex, int role) const
{
	const QVariant src = QIdentityProxyModel::data(proxyIndex, role);
	if (role == Qt::DecorationRole && !src.isNull() && !src.toString().isEmpty())
	{
		return MMC->iconRegistry()->icon(src.toString(), proxyIndex);
	}
	return src;
}

IconProxyModel *IconProxyModel::mixin(QAbstractItemModel *source)
{
	IconProxyModel *model = new IconProxyModel(source);
	model->setSourceModel(source);
	connect(source, &QObject::destroyed, model, &IconProxyModel::resetSourceModel);
	return model;
}

void IconProxyModel::resetSourceModel()
{
	setSourceModel(nullptr);
}
