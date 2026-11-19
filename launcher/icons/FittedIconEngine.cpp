/* Copyright 2026 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */
#include "FittedIconEngine.h"
#include "IconUtils.h"

#include <QPainter>
#include <QStyle>

FittedIconEngine::FittedIconEngine(const QIcon &icon) : m_icon(icon) {}

void FittedIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    // the fitted image in the middle of the rectangle, in device pixels
    const qreal ratio = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
    const QPixmap pixmap = IconUtils::fitted(m_icon, rect.size(), ratio, mode, state);
    if (pixmap.isNull())
        return;
    const QSize shown = pixmap.size() / ratio;
    painter->drawPixmap(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, shown, rect), pixmap);
}

QPixmap FittedIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    // in the pixels asked for: QIcon sets the device pixel ratio on what it hands out
    QPixmap pixmap = IconUtils::fitted(m_icon, size, 1.0, mode, state);
    pixmap.setDevicePixelRatio(1.0);
    return pixmap;
}

QSize FittedIconEngine::actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    // as big as fits, so the shape: from the fitted image (cached for when it's drawn)
    return IconUtils::fitted(m_icon, size, 1.0, mode, state).size();
}

QList<QSize> FittedIconEngine::availableSizes(QIcon::Mode, QIcon::State) const
{
    // any size: window systems then ask for the ones they use
    return {};
}

QString FittedIconEngine::key() const
{
    return QStringLiteral("FittedIconEngine");
}

QIconEngine *FittedIconEngine::clone() const
{
    return new FittedIconEngine(*this);
}
