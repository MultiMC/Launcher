#pragma once

#include <QPalette>
#include <QRectF>

class QPainter;

namespace ThemePreview
{
/// A small window in a palette's colours: title bar, a list with a selected row, a field, a
/// button and a link. For theme cards and the accent colour dialog; about 132 x 84.
void paint(QPainter &painter, const QRectF &rect, const QPalette &palette);
}
