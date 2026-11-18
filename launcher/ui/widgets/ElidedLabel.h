/* Copyright 2026 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QLabel>

/**
 * A plain text label that can get narrower than its text: what doesn't fit ends in "…", and
 * the full text is in its tooltip. A QLabel asks for its whole text's width as its minimum,
 * which made the main window's status bar widen the window to fit long texts (GH-4094).
 */
class ElidedLabel : public QLabel
{
    Q_OBJECT
public:
    using QLabel::QLabel;

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
};
