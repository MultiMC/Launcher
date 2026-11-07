#pragma once
#include <QtGui/QColor>
#include <MessageLevel.h>
#include <QMap>

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
    LogColorCache(QColor front, QColor back)
        : ColorCache(front, back, 1.0)
    {
        addColor((int)MessageLevel::Launcher, QColor("purple"));
        addColor((int)MessageLevel::Debug, QColor("green"));
        addColor((int)MessageLevel::Warning, QColor("orange"));
        addColor((int)MessageLevel::Error, QColor("red"));
        addColor((int)MessageLevel::Fatal, QColor("red"));
        addColor((int)MessageLevel::Message, front);
    }

    QColor getFront(MessageLevel::Enum level)
    {
        if(!m_colors.contains((int) level))
        {
            return ColorCache::getFront((int)MessageLevel::Message);
        }
        return ColorCache::getFront((int)level);
    }

    QColor getBack(MessageLevel::Enum level)
    {
        if(level == MessageLevel::Fatal)
        {
            return QColor(Qt::black);
        }
        return QColor(Qt::transparent);
    }
};
