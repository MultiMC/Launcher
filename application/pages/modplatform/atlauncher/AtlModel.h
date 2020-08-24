#pragma once

#include <QAbstractListModel>

#include "net/NetJob.h"
#include <QIcon>
#include <modplatform/atlauncher/ATLPackIndex.h>

namespace Atl {

class ListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ListModel(QObject *parent);
    virtual ~ListModel();

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void request();

private slots:
    void requestFinished();
    void requestFailed(QString reason);

private:
    QList<ATLauncher::IndexedPack> modpacks;

    NetJobPtr jobPtr;
    QByteArray response;
};

}
