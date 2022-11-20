/*
 * Copyright 2022 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "DescriptionDocument.h"

#include <HoeDown.h>
#include <QPixmapCache>
#include <QDebug>
#include <net/NetJob.h>
#include <Application.h>

Modplatform::DescriptionDocument::DescriptionDocument(const QString &markdown, QObject* parent) : QTextDocument(parent) {
    HoeDown hoedown;
    // 100 MiB
    QPixmapCache::setCacheLimit(102400);
    setHtml(hoedown.process(markdown.toUtf8()));
}

QVariant Modplatform::DescriptionDocument::loadResource(int type, const QUrl& name) {
    if(type == QTextDocument::ResourceType::ImageResource) {
        auto pixmap = QPixmapCache::find(name.toString());
        if(!pixmap) {
            requestResource(name);
            return QVariant();
        }
        return QVariant(*pixmap);
    }
    return QTextDocument::loadResource(type, name);
}

void Modplatform::DescriptionDocument::downloadFinished(const QString& key, const QPixmap& out) {
    m_loading.remove(key);
    QPixmapCache::insert(key, out);
    emit layoutUpdateRequired();
}

void Modplatform::DescriptionDocument::downloadFailed(const QString& key) {
    m_failed.append(key);
    m_loading.remove(key);
}

void Modplatform::DescriptionDocument::requestResource(const QUrl& url) {
    QString key = url.toString();
    if(m_loading.contains(key) || m_failed.contains(key))
    {
        return;
    }

    qDebug() << "Loading resource" << key;

    ImageLoad *load = new ImageLoad;
    load->job = new NetJob(QString("Document Image Download %1").arg(key), APPLICATION->network());
    load->job->addNetAction(Net::Download::makeByteArray(url, &load->output));
    load->key = key;

    QObject::connect(load->job.get(), &NetJob::succeeded, this, [this, load] {
        QPixmap pixmap;
        if(!pixmap.loadFromData(load->output)) {
            qDebug() << load->output;
            downloadFailed(load->key);
        }
        if(pixmap.width() > 800) {
            pixmap = pixmap.scaledToWidth(800);
        }
        downloadFinished(load->key, pixmap);
    });

    QObject::connect(load->job.get(), &NetJob::failed, this, [this, load]
    {
        downloadFailed(load->key);
    });

    load->job->start();

    m_loading[key] = load;
}
