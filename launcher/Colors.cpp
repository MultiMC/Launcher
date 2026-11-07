#include "Colors.h"

#include <QtMath>

namespace {
// sRGB component to linear light
qreal linear(qreal component)
{
    return component <= 0.03928 ? component / 12.92 : qPow((component + 0.055) / 1.055, 2.4);
}

// Black and white text have the same contrast on a colour of this luminance:
// (1 + 0.05) / (L + 0.05) == (L + 0.05) / (0 + 0.05)
const qreal EQUAL_CONTRAST_LUMINANCE = qSqrt(1.05 * 0.05) - 0.05;
}

qreal Colors::luminance(const QColor &color)
{
    return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
}

qreal Colors::contrastRatio(const QColor &a, const QColor &b)
{
    qreal la = luminance(a), lb = luminance(b);
    return (qMax(la, lb) + 0.05) / (qMin(la, lb) + 0.05);
}

bool Colors::isDark(const QColor &color)
{
    return luminance(color) < EQUAL_CONTRAST_LUMINANCE;
}

QColor Colors::mix(const QColor &a, const QColor &b, qreal amount)
{
    if (!(amount > 0.0)) // also NaN
        return a;
    if (amount >= 1.0)
        return b;
    auto between = [amount](qreal x, qreal y) { return x + (y - x) * amount; };
    return QColor::fromRgbF(between(a.redF(), b.redF()), between(a.greenF(), b.greenF()), between(a.blueF(), b.blueF()),
                            between(a.alphaF(), b.alphaF()));
}

QColor Colors::readable(const QColor &color, const QColor &background, qreal minContrast)
{
    if (contrastRatio(color, background) >= minContrast)
        return color;

    // Lighter on dark backgrounds, darker on bright ones: with hue and saturation fixed,
    // luminance only grows with HSL lightness, so a binary search finds the smallest change
    const bool lighter = isDark(background);
    const QColor hsl = color.toHsl();
    const qreal hue = qMax<qreal>(0.0, hsl.hslHueF()); // -1 for greys, where it doesn't matter
    const qreal saturation = hsl.hslSaturationF();
    auto withLightness = [&](qreal lightness) {
        return QColor::fromHslF(hue, saturation, lightness, color.alphaF()).toRgb();
    };

    qreal from = hsl.lightnessF();
    qreal to = lighter ? 1.0 : 0.0;
    if (contrastRatio(withLightness(to), background) < minContrast)
        return withLightness(to);
    for (int i = 0; i < 16; i++)
    {
        qreal middle = (from + to) / 2;
        if (contrastRatio(withLightness(middle), background) >= minContrast)
            to = middle;
        else
            from = middle;
    }
    return withLightness(to);
}
