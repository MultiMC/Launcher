#include "IconDownloader.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <QImageReader>

#include <QListWidgetItem>
#include <QTreeWidgetItem>

AbstractIconDownloader::AbstractIconDownloader(const QUrl& url, QObject *parent) :
	QObject(parent),
	m_nam(new QNetworkAccessManager(this))
{
	connect(m_nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(finished(QNetworkReply*)));
	m_nam->get(QNetworkRequest(url));
}

AbstractIconDownloader::~AbstractIconDownloader()
{
	delete m_nam;
}

void AbstractIconDownloader::finished(QNetworkReply *reply)
{
	setIcon(QIcon(QPixmap::fromImageReader(new QImageReader(reply))));
	reply->deleteLater();
	deleteLater();
}


ListWidgetIconDownloader::ListWidgetIconDownloader(QListWidgetItem *item, const QUrl &url, QObject *parent)
	: AbstractIconDownloader(url, parent), item(item)
{

}
void ListWidgetIconDownloader::setIcon(const QIcon &icon)
{
	item->setIcon(icon);
}

TreeWidgetIconDownloader::TreeWidgetIconDownloader(QTreeWidgetItem *item, const QUrl &url, QObject *parent)
	: AbstractIconDownloader(url, parent), item(item)
{

}
void TreeWidgetIconDownloader::setIcon(const QIcon &icon)
{
	item->setIcon(0, icon);
}
