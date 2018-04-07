#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <modplatform/ftb/PackHelpers.h>
#include <QThreadPool>

#include <RWStorage.h>

#include <QIcon>

typedef QMap<QString, QIcon> FtbLogoMap;
typedef std::function<void(QString)> LogoCallback;

class FtbFilterModel : public QSortFilterProxyModel
{
public:
	FtbFilterModel(QObject* parent = Q_NULLPTR);
	enum Sorting {
		ByName,
		ByGameVersion
	};
	const QMap<QString, Sorting> getAvailableSortings();
	QString translateCurrentSorting();
	void setSorting(Sorting sorting);
	Sorting getCurrentSorting();

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
	bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
	QMap<QString, Sorting> sortings;
	Sorting currentSorting;

};

class FtbListModel : public QAbstractListModel
{
	Q_OBJECT
private:
	FtbModpackList modpacks;
	QStringList m_failedLogos;
	QStringList m_loadingLogos;
	FtbLogoMap m_logoMap;
	QMap<QString, LogoCallback> waitingCallbacks;

	void requestLogo(QString file);
	QString translatePackType(FtbPackType type) const;


private slots:
	void logoFailed(QString logo);
	void logoLoaded(QString logo, QIcon out);

public:
	FtbListModel(QObject *parent);
	~FtbListModel();
	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;

	void fill(FtbModpackList modpacks);

	FtbModpack at(int row);
	void getLogo(const QString &logo, LogoCallback callback);
};
