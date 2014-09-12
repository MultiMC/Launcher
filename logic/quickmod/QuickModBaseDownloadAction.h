#pragma once

#include "logic/net/NetAction.h"

typedef std::shared_ptr<class QuickModBaseDownloadAction> QuickModBaseDownloadActionPtr;
class NetJob;

class QuickModBaseDownloadAction : public NetAction
{
	Q_OBJECT
public:
	explicit QuickModBaseDownloadAction(const QUrl &url);
	static QuickModBaseDownloadActionPtr make(NetJob *netjob, const QUrl &url,
											  const QString &uid = QString(),
											  const QByteArray &checksum = QByteArray());
	virtual ~QuickModBaseDownloadAction()
	{
	}

public:
	QString m_errorString;

	QUrl m_originalUrl;
	QByteArray m_expectedChecksum;

public slots:
	void start() override;

protected slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadError(QNetworkReply::NetworkError error);
	void downloadFinished();
	void downloadReadyRead()
	{
	}

protected:
	virtual bool handle(const QByteArray &data) = 0;
};
