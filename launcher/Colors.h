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

/// The background's hue and saturation at this HSL lightness (0..1)
QColor inHueOf(const QColor &background, qreal lightness);

/// A disabled colour: faded to the background, not to grey. The colour's own hue goes, the
/// background's takes its place (a red theme fades to red, a dark one to its window's grey);
/// only the lightness sits partway from the colour's to the background's (amount 0 is the
/// colour's, 1 the background's). A plain mix would make in-between hues: blue on red purple.
QColor disabled(const QColor &color, const QColor &background, qreal amount);

/// color, lighter or darker (same hue and saturation) just enough to reach minContrast
/// against background; white or black when even that isn't enough. 4.5 is WCAG's minimum
/// for normal text.
QColor readable(const QColor &color, const QColor &background, qreal minContrast = 4.5);

/// An accent colour and its shades, like Windows makes them (Light3 to Dark3); invalid colours
/// when there is none
struct Accent
{
    QColor light3, light2, light1, color, dark1, dark2, dark3;

    bool isValid() const
    {
        return color.isValid();
    }
    bool operator==(const Accent &other) const;
    bool operator!=(const Accent &other) const
    {
        return !(*this == other);
    }

    /// Shades of our own, towards white and black, where only the colour is known
    static Accent fromColor(const QColor &color);
};
}
