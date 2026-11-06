#include "TierLever.h"

#include <QApplication>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>

#include <algorithm>
#include <cmath>

namespace {
const qreal SlotWidth = 10;
const qreal HandleWidth = 36;
const qreal HandleHeight = 20;
const qreal TickLength = 7;
const int DetentSpacing = 44;
}

TierLever::TierLever(int positions, QWidget *parent)
    : QWidget(parent), m_positions(std::max(positions, 2))
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_animation = new QVariantAnimation(this);
    m_animation->setDuration(180);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value)
    {
        m_handle = value.toReal();
        update();
    });
}

void TierLever::setPosition(int position, bool animated)
{
    moveTo(std::max(0, std::min(position, m_positions - 1)), animated);
}

QSize TierLever::sizeHint() const
{
    return minimumSizeHint();
}

QSize TierLever::minimumSizeHint() const
{
    return QSize(int(HandleWidth + 2 * TickLength) + 4, DetentSpacing * m_positions);
}

// The detents are at the middle of equal parts of the height, so they line up with rows of
// equal height next to the lever
qreal TierLever::detentY(qreal position) const
{
    return height() * (position + 0.5) / m_positions;
}

qreal TierLever::positionAt(qreal y) const
{
    return y * m_positions / std::max(height(), 1) - 0.5;
}

int TierLever::nearest(qreal y) const
{
    int position = int(std::floor(y * m_positions / std::max(height(), 1)));
    return std::max(0, std::min(position, m_positions - 1));
}

QRectF TierLever::slotRect() const
{
    qreal top = detentY(0) - SlotWidth;
    qreal bottom = detentY(m_positions - 1) + SlotWidth;
    return QRectF(width() / 2.0 - SlotWidth / 2, top, SlotWidth, bottom - top);
}

QRectF TierLever::handleRect(qreal y) const
{
    return QRectF(width() / 2.0 - HandleWidth / 2, y - HandleHeight / 2, HandleWidth, HandleHeight);
}

void TierLever::moveTo(int position, bool animated)
{
    m_position = position;
    m_animation->stop();
    if(animated && isVisible())
    {
        m_animation->setStartValue(m_handle);
        m_animation->setEndValue(qreal(position));
        m_animation->start();
    }
    else
    {
        m_handle = position;
        update();
    }
}

void TierLever::userMoveTo(int position)
{
    position = std::max(0, std::min(position, m_positions - 1));
    bool changed = position != m_position;
    moveTo(position, true);
    if(changed)
    {
        emit positionChanged(position);
    }
}

void TierLever::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    auto pal = palette();
    auto slot = slotRect();
    auto handleY = detentY(m_handle);
    // as in a selected list item: on macOS the plain Highlight is the pale text selection color
    auto highlight = QApplication::palette("QAbstractItemView").color(pal.currentColorGroup(), QPalette::Highlight);

    // the slot, sunken, filled in the highlight color from the bottom up to the handle
    p.setPen(QPen(pal.color(QPalette::Mid), 1));
    p.setBrush(pal.color(QPalette::Base));
    p.drawRoundedRect(slot, SlotWidth / 2, SlotWidth / 2);
    QRectF fill(slot.left(), handleY, slot.width(), slot.bottom() - handleY);
    QPainterPath slotPath;
    slotPath.addRoundedRect(slot.adjusted(1, 1, -1, -1), SlotWidth / 2 - 1, SlotWidth / 2 - 1);
    p.save();
    p.setClipPath(slotPath);
    p.fillRect(fill, highlight);
    p.restore();

    // a tick on both sides of each detent; the selected one in the highlight color
    for(int i = 0; i < m_positions; i++)
    {
        qreal y = detentY(i);
        p.setPen(QPen(i == m_position ? highlight : pal.color(QPalette::Mid), 2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(1, y), QPointF(1 + TickLength, y));
        p.drawLine(QPointF(width() - 1 - TickLength, y), QPointF(width() - 1, y));
    }

    // the handle, raised, with a grip; outlined in the highlight color while it has focus
    auto handle = handleRect(handleY);
    QLinearGradient gradient(handle.topLeft(), handle.bottomLeft());
    gradient.setColorAt(0, pal.color(QPalette::Light));
    gradient.setColorAt(1, pal.color(QPalette::Button));
    p.setPen(QPen(hasFocus() ? highlight : pal.color(QPalette::Dark), hasFocus() ? 2 : 1));
    p.setBrush(gradient);
    p.drawRoundedRect(handle.adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
    p.setPen(QPen(pal.color(QPalette::Mid), 1));
    for(int i = -1; i <= 1; i++)
    {
        qreal y = handle.center().y() + i * 4;
        p.drawLine(QPointF(handle.left() + 9, y), QPointF(handle.right() - 9, y));
    }
}

void TierLever::mousePressEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    auto handleY = detentY(m_handle);
    if(handleRect(handleY).contains(event->pos()))
    {
        m_animation->stop();
        m_dragging = true;
        m_dragOffset = event->pos().y() - handleY;
    }
    else
    {
        userMoveTo(nearest(event->pos().y()));
    }
}

void TierLever::mouseMoveEvent(QMouseEvent *event)
{
    if(!m_dragging)
    {
        QWidget::mouseMoveEvent(event);
        return;
    }
    m_handle = std::max(qreal(0), std::min(positionAt(event->pos().y() - m_dragOffset), qreal(m_positions - 1)));
    update();
}

void TierLever::mouseReleaseEvent(QMouseEvent *event)
{
    if(!m_dragging || event->button() != Qt::LeftButton)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    m_dragging = false;
    userMoveTo(qRound(m_handle));
}

void TierLever::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Qt::Key_Up:
            userMoveTo(m_position - 1);
            break;
        case Qt::Key_Down:
            userMoveTo(m_position + 1);
            break;
        case Qt::Key_Home:
            userMoveTo(0);
            break;
        case Qt::Key_End:
            userMoveTo(m_positions - 1);
            break;
        default:
            QWidget::keyPressEvent(event);
    }
}
