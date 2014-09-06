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

#pragma once

#include "NetAction.h"

typedef std::shared_ptr<class ByteArrayUpload> ByteArrayUploadPtr;
class ByteArrayUpload : public NetAction
{
	Q_OBJECT
public:
	ByteArrayUpload(const QUrl &url, const QByteArray &data);
	static ByteArrayUploadPtr make(const QUrl &url, const QByteArray &data)
	{
		return std::make_shared<ByteArrayUpload>(url, data);
	}
	virtual ~ByteArrayUpload() {}
public:
	/// if not saving to file, downloaded data is placed here
	QByteArray m_data;

	QByteArray m_dataOut;

	QString m_errorString;

public
slots:
	virtual void start();

protected
slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadError(QNetworkReply::NetworkError error);
	void downloadFinished();
	void downloadReadyRead();
};
