#pragma once

#include "FtbPrivatePackListModel.h"
#include "FtbListModel.h"

class FtbPrivatePackFilterModel : public FtbFilterModel
{
public:
	FtbPrivatePackFilterModel(QObject *parent = Q_NULLPTR);
	bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class FtbPrivatePackListModel : public FtbListModel
{

	Q_OBJECT

public:
	FtbPrivatePackListModel(QObject *parent = Q_NULLPTR);
	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent) const override;

	bool isAddIndex(const QModelIndex &index);
	void addPack(FtbModpack pack);
	void clear();
};
