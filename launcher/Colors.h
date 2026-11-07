#pragma once

#include <QColor>

/// Colour arithmetic for themes and text colours, on WCAG's definitions of luminance and
/// contrast (https://www.w3.org/TR/WCAG21/#dfn-relative-luminance)
namespace Colors
{
/// Relative luminance: 0 for black, 1 for white
qreal luminance(const QColor &color);

/// Contrast ratio between two colours: 1 (none) to 21 (black and white), either order
qreal contrastRatio(const QColor &a, const QColor &b);

/// Whether white text contrasts better on this colour than black
bool isDark(const QColor &color);

/// a and b mixed, alpha included: amount 0 is a, 1 is b
QColor mix(const QColor &a, const QColor &b, qreal amount);

/// color, lighter or darker (same hue and saturation) just enough to reach minContrast
/// against background; white or black when even that isn't enough. 4.5 is WCAG's minimum
/// for normal text.
QColor readable(const QColor &color, const QColor &background, qreal minContrast = 4.5);
}
