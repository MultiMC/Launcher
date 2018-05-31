#include "FtbPrivatePackListModel.h"
#include <QDebug>
#include <QPushButton>
#include "MultiMC.h"
#include "icons/IconList.h"

FtbPrivatePackFilterModel::FtbPrivatePackFilterModel(QObject *parent) : FtbFilterModel(parent)
{
}

bool FtbPrivatePackFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	if(sourceModel()->metaObject()->className() == QString("FtbPrivatePackListModel").toStdString().c_str()) {
		FtbPrivatePackListModel *model = static_cast<FtbPrivatePackListModel*>(sourceModel());
		if(model->isAddIndex(right)) {
			return true;
		}
	}
	return FtbFilterModel::lessThan(left, right);
}

FtbPrivatePackListModel::FtbPrivatePackListModel(QObject *parent) : FtbListModel(parent)
{
}

QVariant FtbPrivatePackListModel::data(const QModelIndex &index, int role) const
{
	if(index.row() == modpacks.size()) {
		if(role == Qt::DisplayRole) {
			return "Add packs";
		} else if(role == Qt::DecorationRole) {
			return MMC->icons()->getIcon("ftb_logo");
		}

		return QVariant();
	}
	return FtbListModel::data(index, role);
}

int FtbPrivatePackListModel::rowCount(const QModelIndex &parent) const
{
	return modpacks.size() + 1;
}

bool FtbPrivatePackListModel::isAddIndex(const QModelIndex &index)
{
	return index.row() == modpacks.size();
}

void FtbPrivatePackListModel::addPack(FtbModpack pack)
{
	beginResetModel();
	modpacks.append(pack);
	endResetModel();
}

void FtbPrivatePackListModel::clear()
{
	beginResetModel();
	modpacks.clear();
	endResetModel();
}
