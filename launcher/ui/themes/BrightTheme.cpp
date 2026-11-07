#include "BrightTheme.h"

QString BrightTheme::id()
{
    return "bright";
}

QString BrightTheme::name()
{
    return QObject::tr("Bright");
}

QPalette BrightTheme::palette()
{
    return colors();
}

// Neutral light colours, Windows 11's (window #f3f3f3, white fields, text #1b1b1b): clean next
// to other apps. Fixed, whatever the style: a style's standard palette can be the desktop's
// colours (Breeze gives the KDE colour scheme), which Bright must not follow. Also the System
// theme's colours on a light system that doesn't give its own (SystemTheme.cpp).
QPalette BrightTheme::colors()
{
    const QColor window(0xf3, 0xf3, 0xf3), button(0xfb, 0xfb, 0xfb), text(0x1b, 0x1b, 0x1b);
    QPalette p(button, window);
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::AlternateBase, QColor(0xf9, 0xf9, 0xf9));
    p.setColor(QPalette::Button, button);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::ToolTipBase, Qt::white);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::BrightText, QColor(0xc4, 0x2b, 0x1c));
    p.setColor(QPalette::Link, QColor(0x00, 0x5f, 0xb8));
    p.setColor(QPalette::LinkVisited, QColor(0x5c, 0x2d, 0x91));
    p.setColor(QPalette::Highlight, QColor(0x00, 0x78, 0xd4));
    p.setColor(QPalette::HighlightedText, Qt::white);
    return p;
}
