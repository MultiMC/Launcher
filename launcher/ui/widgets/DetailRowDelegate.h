#pragma once

#include <QStyledItemDelegate>

#include <functional>

/**
 * A list row with a picture on the left, the row's name (Qt::DisplayRole) in bold and a line
 * of details (detailsRole) below it, on the list's usual background and selection. Rows that
 * can't be chosen are faded. The picture comes from a function, so the same rows show themes
 * and icon sets.
 */
class DetailRowDelegate : public QStyledItemDelegate
{
public:
    using PaintPicture = std::function<void(QPainter &, const QRectF &, const QModelIndex &)>;

    DetailRowDelegate(const QSize &pictureSize, int detailsRole, PaintPicture paintPicture, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private: /* data */
    static const int MARGIN = 4;
    QSize m_pictureSize;
    int m_detailsRole;
    PaintPicture m_paintPicture;
};
