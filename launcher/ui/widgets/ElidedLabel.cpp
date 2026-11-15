#include "ElidedLabel.h"

#include <QPainter>
#include <QStyle>

QSize ElidedLabel::minimumSizeHint() const
{
    // room for the "…" alone; the height stays a label's
    const int margins = 2 * (frameWidth() + margin());
    return QSize(fontMetrics().width(QStringLiteral("…")) + margins, QLabel::minimumSizeHint().height());
}

void ElidedLabel::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    // the frame, if the label has one
    drawFrame(&painter);

    // the text, cut to the width there is
    const QRect area = contentsRect().adjusted(margin(), margin(), -margin(), -margin());
    const QString shown = fontMetrics().elidedText(text(), Qt::ElideRight, area.width());
    style()->drawItemText(&painter, area, QStyle::visualAlignment(layoutDirection(), alignment()), palette(), isEnabled(),
                          shown, foregroundRole());

    // the full text where it was cut
    const QString tip = shown == text() ? QString() : text();
    if (toolTip() != tip)
    {
        setToolTip(tip);
    }
}
