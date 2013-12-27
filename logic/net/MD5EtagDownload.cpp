/* Copyright 2013 MultiMC Contributors
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

#include "MultiMC.h"
#include "MD5EtagDownload.h"
#include <pathutils.h>
#include <QCryptographicHash>
#include "logger/QsLog.h"

MD5EtagDownload::MD5EtagDownload(QUrl url, QString target_path, const bool pipeline)
	: NetAction(), m_pipeline(pipeline)
{
	m_url = url;
	m_target_path = target_path;
	m_status = Job_NotStarted;
}

void MD5EtagDownload::start()
{
	QString filename = m_target_path;
	m_output_file.setFileName(filename);
	// if there already is a file and md5 checking is in effect and it can be opened
	if (m_output_file.exists() && m_output_file.open(QIODevice::ReadOnly))
	{
		// get the md5 of the local file.
		m_local_md5 =
			QCryptographicHash::hash(m_output_file.readAll(), QCryptographicHash::Md5)
				.toHex()
				.constData();
		m_output_file.close();
		// if we are expecting some md5sum, compare it with the local one
		if (!m_expected_md5.isEmpty())
		{
			// skip if they match
			if(m_local_md5 == m_expected_md5)
			{
				QLOG_INFO() << "Skipping " << m_url.toString() << ": md5 match.";
				emit succeeded(m_index_within_job);
				return;
			}
		}
		else
		{
			// no expected md5. we use the local md5sum as an ETag
		}
	}
	if (!ensureFilePathExists(filename))
	{
		emit failed(m_index_within_job);
		return;
	}

	QNetworkRequest request(m_url);

	QLOG_INFO() << "Downloading " << m_url.toString() << " got " << m_local_md5;

	if(!m_local_md5.isEmpty())
	{
		QLOG_INFO() << "Got " << m_local_md5;
		request.setRawHeader(QString("If-None-Match").toLatin1(), m_local_md5.toLatin1());
	}
	if(!m_expected_md5.isEmpty())
		QLOG_INFO() << "Expecting " << m_expected_md5;

	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Uncached)");
	request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, m_pipeline);

	// Go ahead and try to open the file.
	// If we don't do this, empty files won't be created, which breaks the updater.
	// Plus, this way, we don't end up starting a download for a file we can't open.
	if (!m_output_file.open(QIODevice::WriteOnly))
	{
		emit failed(m_index_within_job);
		return;
	}

	auto worker = MMC->qnam();
	QNetworkReply *rep = worker->get(request);

	m_reply = std::shared_ptr<QNetworkReply>(rep);
	connect(rep, SIGNAL(downloadProgress(qint64, qint64)),
			SLOT(downloadProgress(qint64, qint64)));
	connect(rep, SIGNAL(finished()), SLOT(downloadFinished()));
	connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
			SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(rep, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
}

void MD5EtagDownload::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_total_progress = bytesTotal;
	m_progress = bytesReceived;
	emit progress(m_index_within_job, bytesReceived, bytesTotal);
}

void MD5EtagDownload::downloadError(QNetworkReply::NetworkError error)
{
	// error happened during download.
	// TODO: log the reason why
	m_status = Job_Failed;
}

void MD5EtagDownload::downloadFinished()
{
	// if the download succeeded
	if (m_status != Job_Failed)
	{
		// nothing went wrong...
		m_status = Job_Finished;
		m_output_file.close();

		// FIXME: compare with the real written data md5sum
		// this is just an ETag
		QLOG_INFO() << "Finished " << m_url.toString() << " got " << m_reply->rawHeader("ETag").constData();

		m_reply.reset();
		emit succeeded(m_index_within_job);
		return;
	}
	// else the download failed
	else
	{
		m_output_file.close();
		m_output_file.remove();
		m_reply.reset();
		emit failed(m_index_within_job);
		return;
	}
}

void MD5EtagDownload::downloadReadyRead()
{
	if (!m_output_file.isOpen())
	{
		if (!m_output_file.open(QIODevice::WriteOnly))
		{
			/*
			* Can't open the file... the job failed
			*/
			m_reply->abort();
			emit failed(m_index_within_job);
			return;
		}
	}
	m_output_file.write(m_reply->readAll());
}
