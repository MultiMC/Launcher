/* Copyright 2014 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ByteArrayUpload.h"

#include <QBuffer>

#include "MultiMC.h"
#include "logger/QsLog.h"

ByteArrayUpload::ByteArrayUpload(const QUrl &url, const QByteArray &data) : NetAction()
{
	m_url = url;
	m_status = Job_NotStarted;
	m_dataOut = data;
}

void ByteArrayUpload::start()
{
	QLOG_INFO() << "Uploading to" << m_url.toString();
	QNetworkRequest request(m_url);
	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0");
	request.setHeader(QNetworkRequest::ContentTypeHeader, m_content_type);
	QNetworkReply *rep = MMC->qnam()->put(request, m_dataOut);

	m_reply = std::shared_ptr<QNetworkReply>(rep);
	connect(rep, SIGNAL(uploadProgress(qint64, qint64)),
			SLOT(downloadProgress(qint64, qint64)));
	connect(rep, SIGNAL(finished()), SLOT(downloadFinished()));
	connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
			SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(rep, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
}

void ByteArrayUpload::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_total_progress = bytesTotal;
	m_progress = bytesReceived;
	emit progress(m_index_within_job, bytesReceived, bytesTotal);
}

void ByteArrayUpload::downloadError(QNetworkReply::NetworkError error)
{
	// error happened during download.
	QLOG_ERROR() << "Error putting to URL:" << m_url.toString().toLocal8Bit()
				 << "Network error:" << error;
	m_status = Job_Failed;
	m_errorString = m_reply->errorString();
}

void ByteArrayUpload::downloadFinished()
{
	// if the download succeeded
	if (m_status != Job_Failed)
	{
		// nothing went wrong...
		m_status = Job_Finished;
		m_data = m_reply->readAll();
		m_reply.reset();
		emit succeeded(m_index_within_job);
		return;
	}
	// else the download failed
	else
	{
		m_reply.reset();
		emit failed(m_index_within_job);
		return;
	}
}

void ByteArrayUpload::downloadReadyRead()
{
	// ~_~
}
