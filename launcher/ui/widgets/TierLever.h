#pragma once

#include <QWidget>

class QVariantAnimation;

/**
 * A vertical lever with a number of positions (detents), the first at the top. The handle
 * slides between them: click a position, drag the handle (it snaps to the nearest position
 * when let go), or use the arrow keys, Home and End. Painted in the palette's colors, so it
 * follows the theme; the slot below the handle is filled in the highlight color.
 */
class TierLever : public QWidget
{
    Q_OBJECT
public:
    explicit TierLever(int positions, QWidget *parent = nullptr);

    int position() const
    {
        return m_position;
    }
    // without emitting positionChanged
    void setPosition(int position, bool animated = true);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    // the user moved the lever
    void positionChanged(int position);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    qreal detentY(qreal position) const;
    qreal positionAt(qreal y) const;
    int nearest(qreal y) const;
    QRectF slotRect() const;
    QRectF handleRect(qreal y) const;
    void moveTo(int position, bool animated);
    void userMoveTo(int position);

private: /* data */
    int m_positions;
    int m_position = 0;
    // where the handle is, in positions, so resizing doesn't move it; between detents while it moves
    qreal m_handle = 0;
    bool m_dragging = false;
    qreal m_dragOffset = 0;
    QVariantAnimation *m_animation = nullptr;
};
