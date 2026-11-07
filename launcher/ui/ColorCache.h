#pragma once
#include <QtGui/QColor>
#include <MessageLevel.h>
#include "Colors.h"
#include <QMap>
#include <QString>

class ColorCache
{
public:
    ColorCache(QColor front, QColor back, qreal bias)
    {
        m_front = front;
        m_back = back;
        m_bias = bias;
    };

    void addColor(int key, QColor color)
    {
        m_colors[key] = {color, blend(color), blendBackground(color)};
    }

    QColor getFront(int key)
    {
        auto iter = m_colors.find(key);
        if(iter == m_colors.end())
        {
            return QColor();
        }
        return (*iter).front;
    }

    QColor getBack(int key)
    {
        auto iter = m_colors.find(key);
        if(iter == m_colors.end())
        {
            return QColor();
        }
        return (*iter).back;
    }

    /**
     * Make the color readable on the back color, then blend it with the front color
     */
    QColor blend(QColor color);

    /**
     * Blend the color with the back color
     */
    QColor blendBackground(QColor color);

protected:
    struct ColorEntry
    {
        QColor original;
        QColor front;
        QColor back;
    };

protected:
    qreal m_bias;
    QColor m_front;
    QColor m_back;
    QMap<int, ColorEntry> m_colors;
};

class LogColorCache : public ColorCache
{
public:
    /// themeColors: the theme's own, by level name (ITheme::logColors); still made readable
    LogColorCache(QColor front, QColor back, const QMap<QString, QColor> &themeColors = {})
        : ColorCache(front, back, 1.0)
    {
        auto color = [&themeColors](const char *level, QColor ours) { return themeColors.value(level, ours); };
        addColor((int)MessageLevel::Launcher, color("Launcher", QColor("purple")));
        addColor((int)MessageLevel::Debug, color("Debug", QColor("green")));
        addColor((int)MessageLevel::Warning, color("Warning", QColor("orange")));
        addColor((int)MessageLevel::Error, color("Error", QColor("red")));
        addColor((int)MessageLevel::Fatal, color("Fatal", QColor("red")));
        addColor((int)MessageLevel::Message, color("Message", front));
        m_fatalHighlight = color("FatalHighlight", QColor(Qt::black));
    }

    QColor getFront(MessageLevel::Enum level)
    {
        if(!m_colors.contains((int) level))
        {
            return ColorCache::getFront((int)MessageLevel::Message);
        }
        // fatal lines have a background of their own: readable on that
        if(level == MessageLevel::Fatal)
        {
            return Colors::readable(m_colors[(int)level].original, m_fatalHighlight);
        }
        return ColorCache::getFront((int)level);
    }

    QColor getBack(MessageLevel::Enum level)
    {
        if(level == MessageLevel::Fatal)
        {
            return m_fatalHighlight;
        }
        return QColor(Qt::transparent);
    }

private:
    QColor m_fatalHighlight;
};
