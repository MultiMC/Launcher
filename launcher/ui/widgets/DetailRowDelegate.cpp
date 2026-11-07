#include "DetailRowDelegate.h"

#include <QApplication>
#include <QPainter>

DetailRowDelegate::DetailRowDelegate(const QSize &pictureSize, int detailsRole, PaintPicture paintPicture, QObject *parent)
    : QStyledItemDelegate(parent), m_pictureSize(pictureSize), m_detailsRole(detailsRole), m_paintPicture(paintPicture)
{
}

QSize DetailRowDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const
{
    const int text = 2 * option.fontMetrics.height() + MARGIN;
    return QSize(m_pictureSize.width() + 3 * MARGIN + 100, qMax(m_pictureSize.height(), text) + 2 * MARGIN);
}

void DetailRowDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    // the row's background, selection and hover as the style draws them; the text is ours
    opt.text.clear();
    opt.icon = QIcon();
    const QWidget *widget = option.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

    QPainter &p = *painter;
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    const bool enabled = option.state & QStyle::State_Enabled;
    const bool selected = option.state & QStyle::State_Selected;
    const QRect r = option.rect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);

    p.save();
    if (!enabled)
    {
        p.setOpacity(0.4);
    }
    const QRectF picture(QPointF(r.left(), r.top() + (r.height() - m_pictureSize.height()) / 2.0), QSizeF(m_pictureSize));
    m_paintPicture(p, picture, index);
    p.restore();

    const QPalette::ColorGroup group = enabled ? QPalette::Active : QPalette::Disabled;
    const QColor text = option.palette.color(group, selected ? QPalette::HighlightedText : QPalette::Text);
    QColor details = text;
    details.setAlphaF(selected ? 0.8 : 0.65);
    const int textLeft = int(picture.right()) + 2 * MARGIN;
    const int textWidth = r.right() - textLeft;
    const int lineHeight = option.fontMetrics.height();
    const int top = r.top() + (r.height() - 2 * lineHeight - MARGIN / 2) / 2;

    QFont bold = option.font;
    bold.setBold(true);
    p.setFont(bold);
    p.setPen(text);
    const QFontMetrics boldMetrics(bold);
    p.drawText(QRect(textLeft, top, textWidth, lineHeight), Qt::AlignLeft | Qt::AlignVCenter,
               boldMetrics.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, textWidth));
    p.setFont(option.font);
    p.setPen(details);
    p.drawText(QRect(textLeft, top + lineHeight + MARGIN / 2, textWidth, lineHeight), Qt::AlignLeft | Qt::AlignVCenter,
               option.fontMetrics.elidedText(index.data(m_detailsRole).toString(), Qt::ElideRight, textWidth));
    p.restore();
}
