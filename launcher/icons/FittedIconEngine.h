/* Copyright 2026 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */
#pragma once

#include <QIcon>
#include <QIconEngine>

/**
 * An instance icon drawn the way the instance view draws it (IconUtils::fitted) at whatever
 * size is asked for: margins trimmed, as big as fits with its shape kept, small pixel art
 * enlarged sharp. For the places that take a QIcon and draw it themselves: dialogs' icon
 * buttons, window icons, menus. A plain QIcon of a small image stays small in a big button,
 * and a wide one sits thin in a square.
 */
class FittedIconEngine : public QIconEngine
{
public:
    explicit FittedIconEngine(const QIcon &icon);

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QList<QSize> availableSizes(QIcon::Mode mode, QIcon::State state) const override;
    QString key() const override;
    QIconEngine *clone() const override;

private: /* data */
    // the icon as it comes, fit when drawn
    QIcon m_icon;
};
