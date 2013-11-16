#pragma once

#include <QObject>

class QNetworkReply;
class QNetworkAccessManager;
class QListWidgetItem;
class QTreeWidgetItem;

class AbstractIconDownloader : public QObject
{
	Q_OBJECT
public:
	AbstractIconDownloader(const QUrl& url, QObject* parent = 0);
	virtual ~AbstractIconDownloader();

private slots:
	void finished(QNetworkReply* reply);

private:
	QNetworkAccessManager* m_nam;

protected:
	virtual void setIcon(const QIcon& icon) = 0;
};

class ListWidgetIconDownloader : public AbstractIconDownloader
{
	Q_OBJECT
public:
	ListWidgetIconDownloader(QListWidgetItem* item, const QUrl& url, QObject* parent = 0);

private:
	void setIcon(const QIcon& icon);

	QListWidgetItem* item;
};
class TreeWidgetIconDownloader : public AbstractIconDownloader
{
	Q_OBJECT
public:
	TreeWidgetIconDownloader(QTreeWidgetItem* item, const QUrl& url, QObject* parent = 0);

private:
	void setIcon(const QIcon& icon);

	QTreeWidgetItem* item;
};
