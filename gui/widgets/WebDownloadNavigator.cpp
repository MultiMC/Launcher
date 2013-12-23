#include "WebDownloadNavigator.h"

#include <QWebView>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QNetworkReply>

#include "MultiMC.h"

WebDownloadNavigator::WebDownloadNavigator(QWidget *parent)
	: QWidget(parent), m_layout(new QVBoxLayout), m_view(new QWebView(this)),
	  m_bar(new QProgressBar(this))
{
	m_view->setHtml(tr("<h1>Loading...</h1>"));

	connect(m_view, &QWebView::loadProgress, m_bar, &QProgressBar::setValue);
	connect(m_view->page(), &QWebPage::unsupportedContent, this,
			&WebDownloadNavigator::caughtUrl);
	m_view->page()->setForwardUnsupportedContent(true);
	m_view->page()->setNetworkAccessManager(MMC->qnam().get());
	m_view->settings()->setAttribute(QWebSettings::PluginsEnabled, true);

	m_layout->addWidget(m_view, 1);
	m_layout->addWidget(m_bar);
	setLayout(m_layout);
}
void WebDownloadNavigator::load(const QUrl &url)
{
	m_view->load(url);
}
