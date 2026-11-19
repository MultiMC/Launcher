/* Copyright 2026 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */
#include "CappedIconEngine.h"

#include <QApplication>
#include <QCache>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QImageReader>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>

namespace {
// Decoded icons, by file, at most this many kilobytes together: the least recently used go
// first. Generous for a few hundred instance icons (a 512 pixel one is 1 MB, a 128 pixel one
// 64 KB), bounded for a folder of thousands of photos.
QCache<QString, QPixmap> &decoded()
{
    static QCache<QString, QPixmap> cache(64 * 1024);
    return cache;
}

QSize capped(const QSize &size)
{
    if (size.width() <= CappedIconEngine::MAX_SIDE && size.height() <= CappedIconEngine::MAX_SIDE)
        return size;
    return size.scaled(CappedIconEngine::MAX_SIDE, CappedIconEngine::MAX_SIDE, Qt::KeepAspectRatio);
}
}

CappedIconEngine::CappedIconEngine(const QString &path) : m_path(path)
{
    m_cacheKey = path + QLatin1Char('|') + QString::number(QFileInfo(path).lastModified().toMSecsSinceEpoch());
    QImageReader reader(path);
    // the header only
    const QSize size = reader.size();
    if (size.isValid() && !size.isEmpty())
        m_size = capped(size);
}

bool CappedIconEngine::null() const
{
    return !m_size.isValid();
}

QPixmap CappedIconEngine::kept() const
{
    if (null())
        return QPixmap();
    if (auto cached = decoded().object(m_cacheKey))
        return *cached;
    QImageReader reader(m_path);
    // decoded straight to the kept size where the format can (JPEG), shrunk right after
    // otherwise: the full image is only ever held while it's read
    reader.setScaledSize(m_size);
    QImage image = reader.read();
    if (image.isNull())
    {
        qWarning() << "Icon" << m_path << "can't be read:" << reader.errorString();
        return QPixmap();
    }
    if (image.size() != m_size)
        image = image.scaled(m_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    auto pixmap = new QPixmap(QPixmap::fromImage(image));
    const int kilobytes = qMax(1, int(qint64(pixmap->width()) * pixmap->height() * 4 / 1024));
    QPixmap out = *pixmap;
    decoded().insert(m_cacheKey, pixmap, kilobytes);
    return out;
}

QSize CappedIconEngine::actualSize(const QSize &size, QIcon::Mode, QIcon::State)
{
    // as a pixmap icon does: its own size, or smaller to fit, shape kept; never bigger
    if (null())
        return QSize();
    if (m_size.width() <= size.width() && m_size.height() <= size.height())
        return m_size;
    return m_size.scaled(size, Qt::KeepAspectRatio);
}

QList<QSize> CappedIconEngine::availableSizes(QIcon::Mode, QIcon::State) const
{
    if (null())
        return {};
    return {m_size};
}

QPixmap CappedIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QPixmap pixmap = kept();
    if (pixmap.isNull())
        return pixmap;
    const QSize fit = actualSize(size, mode, state);
    if (fit != pixmap.size())
        pixmap = pixmap.scaled(fit, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    // greyed out, highlighted...: as the style makes them for any icon
    if (mode != QIcon::Normal)
    {
        QStyleOption option;
        option.palette = QApplication::palette();
        pixmap = QApplication::style()->generatedIconPixmap(mode, pixmap, &option);
    }
    return pixmap;
}

void CappedIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    // the image in the middle of the rectangle, as big as fits, in device pixels
    const qreal ratio = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
    QPixmap px = pixmap(rect.size() * ratio, mode, state);
    if (px.isNull())
        return;
    const QSize shown = px.size() / ratio;
    painter->drawPixmap(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, shown, rect), px);
}

QString CappedIconEngine::key() const
{
    return QStringLiteral("CappedIconEngine");
}

QIconEngine *CappedIconEngine::clone() const
{
    return new CappedIconEngine(*this);
}
