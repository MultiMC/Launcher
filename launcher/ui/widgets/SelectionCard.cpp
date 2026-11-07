#include "SelectionCard.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

SelectionCard::SelectionCard(const QString &id, const QSize &pictureSize, std::function<void()> clicked, QWidget *parent)
    : QWidget(parent), m_id(id), m_pictureSize(pictureSize), m_clicked(clicked)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void SelectionCard::setName(const QString &name)
{
    m_name = name;
    setAccessibleName(name);
    update();
}

void SelectionCard::setSelected(bool selected)
{
    m_selected = selected;
    update();
}

QSize SelectionCard::sizeHint() const
{
    return QSize(m_pictureSize.width() + 2 * MARGIN, m_pictureSize.height() + 2 * MARGIN + fontMetrics().height() + 4);
}

void SelectionCard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QPalette &own = palette();
    if(m_selected || hasFocus())
    {
        auto highlight = own.color(QPalette::Highlight);
        auto fill = highlight;
        fill.setAlpha(m_selected ? 48 : 0);
        p.setPen(QPen(highlight, m_selected ? 2 : 1, m_selected ? Qt::SolidLine : Qt::DotLine));
        p.setBrush(fill);
        p.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), 6, 6);
    }
    p.save();
    if(!isEnabled())
    {
        p.setOpacity(0.4);
    }
    paintPicture(p, QRectF(QPointF((width() - m_pictureSize.width()) / 2.0, MARGIN), QSizeF(m_pictureSize)));
    p.restore();
    p.setPen(own.color(isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::WindowText));
    QRectF nameRect(0, MARGIN + m_pictureSize.height() + 2, width(), fontMetrics().height());
    p.drawText(nameRect, Qt::AlignHCenter | Qt::AlignVCenter, fontMetrics().elidedText(m_name, Qt::ElideRight, width() - 4));
}

void SelectionCard::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        m_clicked();
    }
}

void SelectionCard::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        m_clicked();
        return;
    }
    QWidget::keyPressEvent(event);
}

void SelectionCard::focusInEvent(QFocusEvent *event)
{
    update();
    QWidget::focusInEvent(event);
}

void SelectionCard::focusOutEvent(QFocusEvent *event)
{
    update();
    QWidget::focusOutEvent(event);
}
