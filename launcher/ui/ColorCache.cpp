#include "ColorCache.h"
#include "Colors.h"

/**
 * Make the color readable on the back color, then blend it with the front color
 */
QColor ColorCache::blend(QColor color)
{
    return Colors::mix(m_front, Colors::readable(color, m_back), m_bias);
}

/**
 * Blend the color with the back color
 */
QColor ColorCache::blendBackground(QColor color)
{
    return Colors::mix(m_back, color, m_bias);
}
