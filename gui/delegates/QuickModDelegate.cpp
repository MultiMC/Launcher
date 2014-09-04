#include "QuickModDelegate.h"

QuickModDelegate::QuickModDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

void QuickModDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
							 const QModelIndex &index) const
{
	if (option.state & QStyle::State_Selected)
	{
		// custom painting
	}
	else
	{
		QStyledItemDelegate::paint(painter, option, index);
	}
}
QSize QuickModDelegate::sizeHint(const QStyleOptionViewItem &option,
								 const QModelIndex &index) const
{
	if (option.state & QStyle::State_Selected)
	{
		return QSize(); // custom painting
	}
	else
	{
		return QStyledItemDelegate::sizeHint(option, index);
	}
}
