#pragma once

#include <QAbstractListModel>

#include "modplatform/modpacksch/PackManifest.h"
#include "net/NetJob.h"

namespace Ftb {

typedef QMap<QString, QIcon> LogoMap;
typedef std::function<void(QString)> LogoCallback;

class ListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ListModel(QObject *parent);
    virtual ~ListModel();

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void getLogo(const QString &logo, const QString &logoUrl, LogoCallback callback);
    void searchWithTerm(const QString & term);

private slots:
    void performSearch();
    void searchRequestFinished();
    void searchRequestFailed(QString reason);

    void requestPack();
    void packRequestFinished();
    void packRequestFailed(QString reason);

    void logoFailed(QString logo);
    void logoLoaded(QString logo, QIcon out);

private:
    void requestLogo(QString file, QString url);

private:
    QList<ModpacksCH::Modpack> modpacks;
    QStringList m_failedLogos;
    QStringList m_loadingLogos;
    LogoMap m_logoMap;
    QMap<QString, LogoCallback> waitingCallbacks;

    QString currentSearchTerm;
    enum SearchState {
        None,
        CanPossiblyFetchMore,
        ResetRequested,
        Finished
    } searchState = None;
    NetJobPtr jobPtr;
    int currentPack;
    QList<int> remainingPacks;
    QByteArray response;
};

}
