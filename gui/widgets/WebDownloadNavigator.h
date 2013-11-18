#pragma once

#include <QWidget>
#include <QStackedWidget>

class QNetworkReply;
class QVBoxLayout;
class QWebView;
class QProgressBar;

class WebDownloadNavigatorStack : public QStackedWidget
{
	Q_OBJECT
public:
	WebDownloadNavigatorStack(QWidget *parent = 0);

public
slots:
	void load(const QList<QUrl> &urls);

signals:
	void caughtUrl(QNetworkReply* reply);
};

class WebDownloadNavigator : public QWidget
{
	Q_OBJECT
public:
	WebDownloadNavigator(QWidget *parent = 0);

public
slots:
	void load(const QUrl& url);

signals:
	void caughtUrl(QNetworkReply* reply);

private:
	QVBoxLayout* m_layout;
	QWebView* m_view;
	QProgressBar* m_bar;
};
