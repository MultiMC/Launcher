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

#pragma once

#include "NetAction.h"
#include "HttpMetaCache.h"
#include <QFile>
#include <qcryptographichash.h>

typedef std::shared_ptr<class CacheDownload> CacheDownloadPtr;
class CacheDownload : public NetAction
{
	Q_OBJECT
public:
	MetaEntryPtr m_entry;
	/// if saving to file, use the one specified in this string
	QString m_target_path;
	/// this is the output file, if any
	QFile m_output_file;
	/// the hash-as-you-download
	QCryptographicHash md5sum;
	/// allow http pipeline? (useful for loads of dl's from the same source)
	bool m_pipeline;

public:
	explicit CacheDownload(const QUrl &url, const MetaEntryPtr &entry,
						   const bool pipeline = false);
	static CacheDownloadPtr make(const QUrl &url, const MetaEntryPtr &entry,
								 const bool pipeline = false)
	{
		return CacheDownloadPtr(new CacheDownload(url, entry, pipeline));
	}

protected
slots:
	virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	virtual void downloadError(QNetworkReply::NetworkError error);
	virtual void downloadFinished();
	virtual void downloadReadyRead();

public
slots:
	virtual void start();
};
