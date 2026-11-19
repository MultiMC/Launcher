/* Copyright 2026 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */
#pragma once

#include <QIconEngine>
#include <QSize>
#include <QString>

/**
 * An icon from an image file, cheap however big the file is: only its header is read up
 * front; the image is decoded when first drawn, straight to at most MAX_SIDE pixels on its long
 * side, and kept in a cache of bounded size (dropped again under pressure, and decoded anew
 * when needed). A plain QIcon of the file decodes all of it and keeps it for good: a folder of
 * photos as the icons folder took gigabytes (GH-2317), a 2362 pixel pack icon crashed (GH-2811).
 *
 * For one-image raster files (PNG, JPEG, GIF's first frame, WebP...); vector and multi-size
 * files (SVG, ICO) are plain QIcons (IconUtils::fromFile).
 */
class CappedIconEngine : public QIconEngine
{
public:
    /// The longest side an icon is kept at: 256 pixel icons on a 2x screen
    static const int MAX_SIDE = 512;

    /// The file's header is read here; null() when it isn't a readable image
    explicit CappedIconEngine(const QString &path);

    bool null() const;

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QList<QSize> availableSizes(QIcon::Mode mode, QIcon::State state) const override;
    QString key() const override;
    QIconEngine *clone() const override;

private:
    /// The image at its kept size, from the cache or decoded now; null when it can't be read
    QPixmap kept() const;

private: /* data */
    QString m_path;
    // the file's path and when it was last written: an edited file is read anew
    QString m_cacheKey;
    // the size it's kept at: the file's, at most MAX_SIDE on the long side
    QSize m_size;
};
