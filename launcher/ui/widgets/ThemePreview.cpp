#include "ThemePreview.h"

#include <QPainter>

#include "Colors.h"

void ThemePreview::paint(QPainter &p, const QRectF &r, const QPalette &palette)
{
    const QPalette &c = palette;
    auto color = [&c](QPalette::ColorRole role) { return c.color(QPalette::Active, role); };
    auto line = [&p](const QRectF &rect, const QColor &color) {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(rect, 1.5, 1.5);
    };
    const QColor window = color(QPalette::Window);
    const QColor outline = Colors::isDark(window) ? window.lighter(160) : window.darker(130);

    // the window, its title bar a little apart from the content
    p.setPen(QPen(outline, 1));
    p.setBrush(window);
    p.drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
    const qreal title = 14;
    p.setPen(Qt::NoPen);
    p.setBrush(Colors::isDark(window) ? window.darker(115) : window.darker(104));
    p.drawRoundedRect(QRectF(r.left() + 1, r.top() + 1, r.width() - 2, title), 3, 3);
    line(QRectF(r.left() + 8, r.top() + 6, 36, 3), color(QPalette::WindowText));
    for(int i = 0; i < 3; i++)
    {
        p.setBrush(outline);
        p.drawEllipse(QPointF(r.right() - 10 - i * 9, r.top() + 7.5), 2.5, 2.5);
    }

    // a list with a selected row
    QRectF list(r.left() + 8, r.top() + title + 8, r.width() * 0.55, r.height() - title - 16);
    p.setPen(QPen(outline, 1));
    p.setBrush(color(QPalette::Base));
    p.drawRect(list.adjusted(0.5, 0.5, -0.5, -0.5));
    const qreal row = 11;
    for(int i = 0; i < 4 && list.top() + 3 + (i + 1) * row <= list.bottom(); i++)
    {
        QRectF rowRect(list.left() + 2, list.top() + 2 + i * row, list.width() - 4, row - 1);
        QColor text = color(QPalette::Text);
        if(i == 1)
        {
            line(rowRect, color(QPalette::Highlight));
            text = color(QPalette::HighlightedText);
        }
        line(QRectF(rowRect.left() + 4, rowRect.center().y() - 1.5, rowRect.width() * (i % 2 ? 0.6 : 0.75), 3), text);
    }

    // a field, a button and a link next to it
    const qreal right = list.right() + 8;
    const qreal width = r.right() - 8 - right;
    QRectF field(right, list.top(), width, 13);
    p.setPen(QPen(outline, 1));
    p.setBrush(color(QPalette::Base));
    p.drawRoundedRect(field.adjusted(0.5, 0.5, -0.5, -0.5), 2, 2);
    line(QRectF(field.left() + 4, field.center().y() - 1.5, width * 0.5, 3), color(QPalette::Text));

    QRectF button(right, field.bottom() + 6, width, 15);
    p.setPen(QPen(outline, 1));
    p.setBrush(color(QPalette::Button));
    p.drawRoundedRect(button.adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);
    line(QRectF(button.center().x() - width * 0.25, button.center().y() - 1.5, width * 0.5, 3), color(QPalette::ButtonText));

    line(QRectF(right, button.bottom() + 8, width * 0.7, 3), color(QPalette::Link));

}
