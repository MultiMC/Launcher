#pragma once

#include <QWidget>

#include <functional>

/**
 * A choice shown as a picture with its name below, for visual pickers (themes, icons). The
 * selected one is framed in the highlight color, the focused one dotted; clicking one, or
 * Space or Enter on it, chooses it. Subclasses paint the picture.
 */
class SelectionCard : public QWidget
{
public:
    SelectionCard(const QString &id, const QSize &pictureSize, std::function<void()> clicked, QWidget *parent);

    QString id() const
    {
        return m_id;
    }
    void setName(const QString &name);
    void setSelected(bool selected);

    QSize sizeHint() const override;

protected:
    virtual void paintPicture(QPainter &painter, const QRectF &rect) = 0;

    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private: /* data */
    static const int MARGIN = 6;
    QString m_id;
    QSize m_pictureSize;
    std::function<void()> m_clicked;
    QString m_name;
    bool m_selected = false;
};
