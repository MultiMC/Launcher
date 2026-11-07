#include "DarkTheme.h"

QString DarkTheme::id()
{
    return "dark";
}

QString DarkTheme::name()
{
    return QObject::tr("Dark");
}

QPalette DarkTheme::palette()
{
    return colors();
}

QPalette DarkTheme::colors()
{
    const QColor window(49, 54, 59), blue(42, 130, 218);
    QPalette p(window, window);
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, Qt::white);
    p.setColor(QPalette::Base, QColor(35, 38, 41));
    p.setColor(QPalette::AlternateBase, window);
    // the blue tooltips the Dark theme's stylesheet used to give
    p.setColor(QPalette::ToolTipBase, blue);
    p.setColor(QPalette::ToolTipText, Qt::white);
    p.setColor(QPalette::Text, Qt::white);
    p.setColor(QPalette::Button, window);
    p.setColor(QPalette::ButtonText, Qt::white);
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Link, blue);
    p.setColor(QPalette::LinkVisited, blue);
    p.setColor(QPalette::Highlight, blue);
    p.setColor(QPalette::HighlightedText, Qt::black);
    return p;
}
