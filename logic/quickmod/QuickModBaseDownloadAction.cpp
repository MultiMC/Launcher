#include "QuickModBaseDownloadAction.h"

#include "MultiMC.h"
#include "logger/QsLog.h"

#include "QuickModDownloadAction.h"
#include "QuickModIndexDownloadAction.h"
#include "QuickModsList.h"
#include "QuickModDatabase.h"

/*
 * FIXME: this actually fixes some kind of Qt bug that we should report.
 * The bug leads to segfaults inside QtNetwork when you try to use invalid URLs for downloads.
 * We will have to validate every single URL.
 *
 * Example of invalid URL: github://peterix@quickmods/CodeChickenCore.quickmod
 *
 * TODO: move to net actions?
 */
bool isUrlActuallyValid(const QUrl &url)
{
	auto scheme = url.scheme();
	QLOG_INFO() << "URL " << url << " scheme " << scheme;
	if (scheme == "file")
		return true;
	if (scheme == "http")
		return true;
	if (scheme == "https")
		return true;
	if (scheme == "ftp")
		return true;
	return false;
}

QuickModBaseDownloadAction::QuickModBaseDownloadAction(const QUrl &url) : NetAction()
{
	m_url = m_originalUrl = url;
	m_status = Job_NotStarted;
}

QuickModBaseDownloadActionPtr QuickModBaseDownloadAction::make(NetJob *netjob, const QUrl &url,
															   const QString &uid,
															   const QByteArray &checksum)
{
	QuickModBaseDownloadActionPtr ret;
	if (url.path().endsWith("index.json"))
	{
		ret = std::make_shared<QuickModIndexDownloadAction>(url, netjob);
	}
	else
	{
		ret = std::make_shared<QuickModDownloadAction>(url, uid);
	}
	ret->m_expectedChecksum = checksum;
	return ret;
}

void QuickModBaseDownloadAction::start()
{
	if (!m_url.isValid() || !isUrlActuallyValid(m_url))
	{
		QLOG_ERROR() << "Invalid URL:" << m_url.toString();
		emit failed(m_index_within_job);
		return;
	}
	QLOG_INFO() << "Downloading " << m_url.toString();
	QNetworkRequest request(m_url);
	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Cached)");
	request.setRawHeader("If-None-Match", m_expectedChecksum);
	QNetworkReply *rep = MMC->qnam()->get(request);

	m_reply = std::shared_ptr<QNetworkReply>(rep);
	connect(rep, &QNetworkReply::downloadProgress, this,
			&QuickModBaseDownloadAction::downloadProgress);
	connect(rep, &QNetworkReply::finished, this, &QuickModBaseDownloadAction::downloadFinished);
	connect(rep, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(
					 &QNetworkReply::error),
			this, &QuickModBaseDownloadAction::downloadError);
}

void QuickModBaseDownloadAction::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_total_progress = bytesTotal;
	m_progress = bytesReceived;
	emit progress(m_index_within_job, bytesReceived, bytesTotal);
}

void QuickModBaseDownloadAction::downloadError(QNetworkReply::NetworkError error)
{
	// error happened during download.
	QLOG_ERROR() << "Error getting URL:" << m_url.toString().toLocal8Bit()
				 << "Network error: " << error;
	m_status = Job_Failed;
	m_errorString = m_reply->errorString();
}

void QuickModBaseDownloadAction::downloadFinished()
{
	// redirects
	{
		QVariant redirect = m_reply->header(QNetworkRequest::LocationHeader);
		QString redirectURL;
		if (redirect.isValid())
		{
			redirectURL = redirect.toString();
		}
		// FIXME: This is a hack for https://bugreports.qt-project.org/browse/QTBUG-41061
		else if (m_reply->hasRawHeader("Location"))
		{
			auto data = m_reply->rawHeader("Location");
			if (data.size() > 2 && data[0] == '/' && data[1] == '/')
				redirectURL = m_reply->url().scheme() + ":" + data;
		}
		if (!redirectURL.isEmpty())
		{
			m_url = QUrl(redirect.toString());
			QLOG_INFO() << "Following redirect to " << m_url.toString();
			start();
			return;
		}
	}

	// if the download succeeded
	if (m_status != Job_Failed)
	{
		const QByteArray receivedHash = m_reply->rawHeader("ETag").replace("\"", "");
		if (!receivedHash.isEmpty())
		{
			MMC->quickmodslist()->database()->setChecksum(m_originalUrl, receivedHash);
		}
		if (receivedHash == m_expectedChecksum || handle(m_reply->readAll()))
		{
			// nothing went wrong...
			m_status = Job_Finished;
			emit succeeded(m_index_within_job);
		}
	}
	// else the download failed
	else
	{
		emit failed(m_index_within_job);
	}
	m_reply.reset();
}
