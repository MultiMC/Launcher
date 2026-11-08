/* Copyright 2013-2023 MultiMC Contributors
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

#include "Download.h"

#include <QFileInfo>
#include <QDateTime>
#include <QDebug>

#include "FileSystem.h"
#include "ChecksumValidator.h"
#include "MetaCacheSink.h"
#include "ByteArraySink.h"
#include "Mirrors.h"

#include "BuildConfig.h"

namespace Net {

Download::Download():NetAction()
{
    m_status = Job_NotStarted;
    m_stall_timer.setSingleShot(true);
    connect(&m_stall_timer, &QTimer::timeout, this, [this]()
    {
        qCritical() << "Download" << m_url.toString() << "stalled, giving up";
        stop(QNetworkReply::TimeoutError);
    });
}

Download::Ptr Download::makeCached(QUrl url, MetaEntryPtr entry, Options options)
{
    Download * dl = new Download();
    dl->m_url = url;
    dl->m_options = options;
    auto md5Node = new ChecksumValidator(QCryptographicHash::Md5);
    auto cachedNode = new MetaCacheSink(entry, md5Node);
    dl->m_sink.reset(cachedNode);
    dl->m_target_path = entry->getFullPath();
    return dl;
}

Download::Ptr Download::makeByteArray(QUrl url, QByteArray *output, Options options)
{
    Download * dl = new Download();
    dl->m_url = url;
    dl->m_options = options;
    dl->m_sink.reset(new ByteArraySink(output));
    return dl;
}

Download::Ptr Download::makeFile(QUrl url, QString path, Options options)
{
    Download * dl = new Download();
    dl->m_url = url;
    dl->m_options = options;
    dl->m_sink.reset(new FileSink(path));
    return dl;
}

void Download::addValidator(Validator * v)
{
    m_sink->addValidator(v);
}

void Download::setStallTimeout(int msecs)
{
    m_stall_timer.setInterval(msecs);
}

void Download::stop(QNetworkReply::NetworkError error)
{
    // the reply's abort() comes back through downloadError() and downloadFinished(), which see Job_Failed
    m_status = Job_Failed;
    m_network_error = error;
    m_stall_timer.stop();
    if(m_reply)
    {
        m_reply->abort();
    }
}

void Download::startImpl()
{
    if(m_status == Job_Aborted)
    {
        qWarning() << "Attempt to start an aborted Download:" << m_url.toString();
        emit aborted(m_index_within_job);
        return;
    }
    // every attempt starts again from the first mirror of the URL it was made for
    if(!m_requested_url.isValid())
    {
        m_requested_url = m_url;
    }
    m_mirrors = (m_options & Option::NoMirrors) ? QList<QUrl>{m_requested_url} : Mirrors::candidates(m_requested_url);
    m_mirror = 0;
    m_mirrorSaidNotFound = false;
    m_url = m_mirrors.first();
    startRequest();
}

void Download::startRequest()
{
    m_network_error = QNetworkReply::NoError;
    m_http_status = 0;
    QNetworkRequest request(m_url);
    m_status = m_sink->init(request);
    switch(m_status)
    {
        case Job_Finished:
            emit succeeded(m_index_within_job);
            qDebug() << "Download cache hit " << m_url.toString();
            return;
        case Job_InProgress:
            qDebug() << "Downloading " << m_url.toString();
            break;
        case Job_Failed_Proceed: // this is meaningless in this context. We do need a sink.
        case Job_NotStarted:
        case Job_Failed:
            emit failed(m_index_within_job);
            return;
        case Job_Aborted:
            return;
    }

    request.setHeader(QNetworkRequest::UserAgentHeader, BuildConfig.USER_AGENT);

    QNetworkReply *rep = m_network->get(request);

    m_reply.reset(rep);
    connect(rep, SIGNAL(downloadProgress(qint64, qint64)), SLOT(downloadProgress(qint64, qint64)));
    connect(rep, SIGNAL(finished()), SLOT(downloadFinished()));
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
    connect(rep, &QNetworkReply::sslErrors, this, &Download::sslErrors);
    connect(rep, &QNetworkReply::readyRead, this, &Download::downloadReadyRead);
    if(m_stall_timer.interval() > 0)
    {
        m_stall_timer.start();
    }
}

void Download::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    m_total_progress = bytesTotal;
    m_progress = bytesReceived;
    if(m_stall_timer.isActive())
    {
        m_stall_timer.start();
    }
    emit netActionProgress(m_index_within_job, bytesReceived, bytesTotal);
}

void Download::downloadError(QNetworkReply::NetworkError error)
{
    if(error == QNetworkReply::OperationCanceledError)
    {
        if(m_status == Job_Failed)
        {
            // stopped by us, not aborted: the data was refused, or the transfer stalled
            return;
        }
        qCritical() << "Aborted " << m_url.toString();
        m_status = Job_Aborted;
    }
    else
    {
        m_network_error = error;
        if(m_options & Option::AcceptLocalFiles)
        {
            if(m_sink->hasLocalData())
            {
                m_status = Job_Failed_Proceed;
                return;
            }
        }
        // error happened during download.
        qCritical() << "Failed " << m_url.toString() << " with reason " << error;
        m_status = Job_Failed;
    }
}

void Download::sslErrors(const QList<QSslError> & errors)
{
    int i = 1;
    for (auto error : errors)
    {
        qCritical() << "Download" << m_url.toString() << "SSL Error #" << i << " : " << error.errorString();
        auto cert = error.certificate();
        qCritical() << "Certificate in question:\n" << cert.toText();
        i++;
    }
}

bool Download::handleRedirect()
{
    QUrl redirect = m_reply->header(QNetworkRequest::LocationHeader).toUrl();
    if(!redirect.isValid())
    {
        if(!m_reply->hasRawHeader("Location"))
        {
            // no redirect -> it's fine to continue
            return false;
        }
        // there is a Location header, but it's not correct. we need to apply some workarounds...
        QByteArray redirectBA = m_reply->rawHeader("Location");
        if(redirectBA.size() == 0)
        {
            // empty, yet present redirect header? WTF?
            return false;
        }
        QString redirectStr = QString::fromUtf8(redirectBA);

        if(redirectStr.startsWith("//"))
        {
            /*
             * IF the URL begins with //, we need to insert the URL scheme.
             * See: https://bugreports.qt.io/browse/QTBUG-41061
             * See: http://tools.ietf.org/html/rfc3986#section-4.2
             */
            redirectStr = m_reply->url().scheme() + ":" + redirectStr;
        }
        else if(redirectStr.startsWith("/"))
        {
            /*
             * IF the URL begins with /, we need to process it as a relative URL
             */
            auto url = m_reply->url();
            url.setPath(redirectStr, QUrl::TolerantMode);
            redirectStr = url.toString();
        }

        /*
         * Next, make sure the URL is parsed in tolerant mode. Qt doesn't parse the location header in tolerant mode, which causes issues.
         * FIXME: report Qt bug for this
         */
        redirect = QUrl(redirectStr, QUrl::TolerantMode);
        if(!redirect.isValid())
        {
            qWarning() << "Failed to parse redirect URL:" << redirectStr;
            downloadError(QNetworkReply::ProtocolFailure);
            return false;
        }
        qDebug() << "Fixed location header:" << redirect;
    }
    else
    {
        qDebug() << "Location header:" << redirect;
    }

    m_url = QUrl(redirect.toString());
    qDebug() << "Following redirect to " << m_url.toString();
    startRequest();
    return true;
}

bool Download::tryNextMirror()
{
    // only failures to reach the host at all, not HTTP errors or refused data
    const bool unreachable = m_network_error > QNetworkReply::NoError && m_network_error < QNetworkReply::ProxyConnectionRefusedError
        && m_network_error != QNetworkReply::OperationCanceledError;
    if(unreachable)
    {
        Mirrors::markUnreachable(m_url);
    }
    if(wasNotFound())
    {
        m_mirrorSaidNotFound = true;
    }
    if(m_mirror + 1 >= m_mirrors.size())
    {
        // a mirror that answered knows better than ones that couldn't be reached: the file isn't there
        if(unreachable && m_mirrorSaidNotFound)
        {
            m_network_error = QNetworkReply::ContentNotFoundError;
        }
        return false;
    }
    m_mirror++;
    m_url = m_mirrors[m_mirror];
    qWarning() << "Trying the next mirror:" << m_url.toString();
    startRequest();
    return true;
}


void Download::downloadFinished()
{
    m_stall_timer.stop();
    auto status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    m_http_status = status.isValid() ? status.toInt() : 0;

    // handle HTTP redirection first, unless it was stopped
    if(m_status != Job_Failed && handleRedirect())
    {
        qDebug() << "Download redirected:" << m_url.toString();
        return;
    }

    // if the download failed before this point ...
    if (m_status == Job_Failed_Proceed)
    {
        qDebug() << "Download failed but we are allowed to proceed:" << m_url.toString();
        m_sink->abort();
        m_reply.reset();
        emit succeeded(m_index_within_job);
        return;
    }
    else if (m_status == Job_Failed)
    {
        qDebug() << "Download failed in previous step:" << m_url.toString();
        m_sink->abort();
        m_reply.reset();
        if(!tryNextMirror())
        {
            emit failed(m_index_within_job);
        }
        return;
    }
    else if(m_status == Job_Aborted)
    {
        qDebug() << "Download aborted in previous step:" << m_url.toString();
        m_sink->abort();
        m_reply.reset();
        emit aborted(m_index_within_job);
        return;
    }

    // make sure we got all the remaining data, if any
    auto data = m_reply->readAll();
    if(data.size())
    {
        qDebug() << "Writing extra" << data.size() << "bytes to" << m_target_path;
        m_status = m_sink->write(data);
    }

    // otherwise, finalize the whole graph
    m_status = m_sink->finalize(*m_reply.get());
    if (m_status != Job_Finished)
    {
        qDebug() << "Download failed to finalize:" << m_url.toString();
        m_sink->abort();
        m_reply.reset();
        if(!tryNextMirror())
        {
            emit failed(m_index_within_job);
        }
        return;
    }
    m_reply.reset();
    qDebug() << "Download succeeded:" << m_url.toString();
    emit succeeded(m_index_within_job);
}

void Download::downloadReadyRead()
{
    if(m_status == Job_InProgress)
    {
        auto data = m_reply->readAll();
        // an error page (some servers send a large HTML one with a 404) isn't the file: the
        // error that follows decides, not the validators
        auto status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        if(status.isValid() && status.toInt() >= 400)
        {
            return;
        }
        m_status = m_sink->write(data);
        if(m_status == Job_Failed)
        {
            // nothing more is wanted from this response
            qCritical() << "Failed to process response chunk for " << m_url.toString();
            stop(QNetworkReply::NoError);
        }
        // qDebug() << "Download" << m_url.toString() << "gained" << data.size() << "bytes";
    }
    else
    {
        qCritical() << "Cannot write to " << m_target_path << ", illegal status" << m_status;
    }
}

}

bool Net::Download::abort()
{
    if(m_reply)
    {
        m_reply->abort();
    }
    else
    {
        m_status = Job_Aborted;
    }
    return true;
}

bool Net::Download::canAbort()
{
    return true;
}
