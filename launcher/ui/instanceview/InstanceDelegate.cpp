/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "InstanceDelegate.h"
#include <QPainter>
#include <QTextOption>
#include <QTextLayout>
#include <QApplication>
#include <QtMath>
#include <QDebug>

#include "InstanceView.h"
#include "BaseInstance.h"
#include "InstanceList.h"
#include "icons/IconUtils.h"
#include <xdgicon.h>
#include <QTextEdit>
#include <QResizeEvent>
#include <QTextDocument>

// Origin: Qt
static void viewItemTextLayout(QTextLayout &textLayout, int lineWidth, qreal &height,
                               qreal &widthUsed)
{
    height = 0;
    widthUsed = 0;
    textLayout.beginLayout();
    QString str = textLayout.text();
    while (true)
    {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;
        if (line.textLength() == 0)
            break;
        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    textLayout.endLayout();
}

ListViewDelegate::ListViewDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

namespace {
// space around the icon and the text, inside the item's frame; a little more above the icon
const int PADDING = 4;
const int TOP_PADDING = 7;
// between the icon and the text
const int ICON_TEXT_GAP = 7;

// The view's icon size, from its options as the view gives them: initStyleOption() shrinks
// decorationSize to the icon's own size, which would make a small icon's item smaller
int iconSizeOf(const QStyleOptionViewItem &option)
{
    return option.decorationSize.isValid() ? option.decorationSize.width() : 48;
}

QColor withAlpha(QColor color, qreal alpha)
{
    color.setAlphaF(alpha);
    return color;
}
}

// What's under the icon and the name, like file managers draw it: selected, a frame tinted
// with the accent and edged with it; under the mouse, a faint one in the text's colour; the
// current item, the faint edge alone. Under a frame, and under the name of any other item, a
// half-clear backing in the window's colour keeps the text readable on whatever is behind the
// view (the cat). The icon and the text keep their colours. textBox: where the name is drawn.
void drawItemBackground(QPainter *painter, const QStyleOptionViewItem &option, const QRect &textBox)
{
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    const bool current = option.state & QStyle::State_HasFocus;
    QPalette::ColorGroup group = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if (group == QPalette::Normal && !(option.state & QStyle::State_Active))
        group = QPalette::Inactive;
    const QColor accent = option.palette.color(group, QPalette::Highlight);
    const QColor text = option.palette.color(group, QPalette::Text);
    // as it always was behind the names (160 of 255)
    const QColor backing = withAlpha(option.palette.color(group, QPalette::Window), 0.63);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    const bool framed = selected || hovered;
    if (!framed)
    {
        // the name's backing, just around the name
        painter->setBrush(backing);
        painter->drawRoundedRect(QRectF(textBox).adjusted(-2, -1, 2, 1), 3, 3);
    }
    if (framed || current)
    {
        // the frame: a rounded rectangle on the pixel grid, its 1 pixel edge inside the item;
        // the backing, then the tint, then the edge
        const QRectF frame = QRectF(option.rect).adjusted(0.5, 0.5, -0.5, -0.5);
        if (framed)
        {
            painter->setBrush(backing);
            painter->drawRoundedRect(frame, 4, 4);
            painter->setBrush(selected ? withAlpha(accent, hovered ? 0.45 : 0.3) : withAlpha(text, 0.08));
            painter->drawRoundedRect(frame, 4, 4);
        }
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(selected ? withAlpha(accent, 0.85) : withAlpha(text, 0.2), 1));
        painter->drawRoundedRect(frame, 4, 4);
    }
    painter->restore();
}

// TODO this can be made a lot prettier
void drawProgressOverlay(QPainter *painter, const QStyleOptionViewItem &option,
                         const int value, const int maximum)
{
    if (maximum == 0 || value == maximum)
    {
        return;
    }

    painter->save();

    qreal percent = (qreal)value / (qreal)maximum;
    QColor color = option.palette.color(QPalette::Dark);
    color.setAlphaF(0.70f);
    painter->setBrush(color);
    painter->setPen(QPen(QBrush(), 0));
    painter->drawPie(option.rect, 90 * 16, -percent * 360 * 16);

    painter->restore();
}

// The instance's badges (running or broken, an update), as KDE draws icon overlays: a third of
// the icon's size, each in a corner of it, sticking out a little into the space around it:
// bottom left first, then bottom right, top right, top left (mirrored right to left).
// iconRect: the square the icon is fit in.
void drawBadges(QPainter *painter, const QRect &iconRect, int iconSize, Qt::LayoutDirection direction, BaseInstance *instance,
                QIcon::Mode mode, QIcon::State state)
{
    QList<QString> badges;
    if (instance->isRunning())
    {
        badges.append("status-running");
    }
    else if (instance->hasCrashed() || instance->hasVersionBroken())
    {
        badges.append("status-bad");
    }
    if (instance->hasUpdateAvailable())
    {
        badges.append("checkupdate");
    }

    const int side = qMax(12, iconSize / 3);
    // how far out of the icon: a quarter of a badge, but not over the name below it or past the
    // item's edge above it
    const int outSide = side / 4;
    const int outBelow = qMin(side / 4, ICON_TEXT_GAP - 2);
    const int outAbove = qMin(side / 4, TOP_PADDING - 2);
    const int left = iconRect.left() - outSide;
    const int right = iconRect.right() + 1 + outSide - side;
    const int top = iconRect.top() - outAbove;
    const int bottom = iconRect.bottom() + 1 + outBelow - side;
    const bool rightToLeft = direction == Qt::RightToLeft;
    const QPoint corners[] = {
        {rightToLeft ? right : left, bottom},
        {rightToLeft ? left : right, bottom},
        {rightToLeft ? left : right, top},
        {rightToLeft ? right : left, top},
    };
    for (int i = 0; i < badges.size() && i < 4; i++)
    {
        // FIXME: inject this.
        auto icon = XdgIcon::fromTheme(badges[i]);
        icon.paint(painter, QRect(corners[i], QSize(side, side)), Qt::AlignCenter, mode, state);
    }
}

static QSize viewItemTextSize(const QStyleOptionViewItem *option, int width)
{
    QStyle *style = option->widget ? option->widget->style() : QApplication::style();
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    QTextLayout textLayout;
    textLayout.setTextOption(textOption);
    textLayout.setFont(option->font);
    textLayout.setText(option->text);
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, option, option->widget) + 1;
    QRect bounds(0, 0, width - 2 * textMargin, 600);
    qreal height = 0, widthUsed = 0;
    viewItemTextLayout(textLayout, bounds.width(), height, widthUsed);
    const QSize size(qCeil(widthUsed), qCeil(height));
    return QSize(size.width() + 2 * textMargin, size.height());
}

void ListViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    painter->save();
    painter->setClipRect(opt.rect);

    opt.features |= QStyleOptionViewItem::WrapText;
    opt.text = index.data().toString();
    opt.textElideMode = Qt::ElideRight;
    opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

    // the icon on top, the name under it, all inside the frame's padding
    const int iconSize = iconSizeOf(option);
    QRect iconbox = opt.rect.adjusted(PADDING, TOP_PADDING, -PADDING, 0);
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, opt.widget) + 1;
    QRect textRect = opt.rect;
    textRect.adjust(textMargin + PADDING, TOP_PADDING + iconSize + ICON_TEXT_GAP, -textMargin - PADDING, -PADDING);

    // the name laid out first: its backing goes under it
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textOption.setTextDirection(opt.direction);
    textOption.setAlignment(QStyle::visualAlignment(opt.direction, opt.displayAlignment));
    QTextLayout textLayout;
    textLayout.setTextOption(textOption);
    textLayout.setFont(opt.font);
    textLayout.setText(opt.text);

    qreal width, height;
    viewItemTextLayout(textLayout, textRect.width(), height, width);

    const int lineCount = textLayout.lineCount();

    const QRect layoutRect = QStyle::alignedRect(
        opt.direction, opt.displayAlignment, QSize(textRect.width(), int(height)), textRect);
    const QRect textBox = QStyle::alignedRect(opt.direction, Qt::AlignHCenter | Qt::AlignTop, QSize(qCeil(width), int(height)), layoutRect);

    // the frame or the name's backing: selection, hover, the current item
    drawItemBackground(painter, opt, textBox);

    // icon mode and state, also used for badges
    // (not Selected: that tints the whole icon in the accent; the frame shows the selection)
    QIcon::Mode mode = QIcon::Normal;
    if (!(opt.state & QStyle::State_Enabled))
        mode = QIcon::Disabled;
    QIcon::State state = opt.state & QStyle::State_Open ? QIcon::On : QIcon::Off;

    // the icon, fit to the square (IconUtils::fitted)
    {
        iconbox.setHeight(iconSize);
        const qreal ratio = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
        const QPixmap pixmap = IconUtils::fitted(opt.icon, QSize(iconSize, iconSize), ratio, mode, state);
        if (!pixmap.isNull())
        {
            const QSize shown = pixmap.size() / pixmap.devicePixelRatio();
            painter->drawPixmap(QStyle::alignedRect(opt.direction, Qt::AlignCenter, shown, iconbox), pixmap);
        }
    }
    // set the text colors
    QPalette::ColorGroup cg =
        opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    // the frame is a light tint: the text keeps its colour on it
    painter->setPen(opt.palette.color(cg, QPalette::Text));

    // the name
    const QPointF position = layoutRect.topLeft();
    for (int i = 0; i < lineCount; ++i)
    {
        const QTextLine line = textLayout.lineAt(i);
        line.draw(painter, position);
    }

    // the badges (running, crashed, update) on the icon's bottom corner
    // FIXME: this really has no business of being here. Make generic.
    auto instance = (BaseInstance*)index.data(InstanceList::InstancePointerRole)
            .value<void *>();
    if (instance)
    {
        // on the square the icon is fit in, so they're in the same places on every item
        drawBadges(painter, QStyle::alignedRect(opt.direction, Qt::AlignCenter, QSize(iconSize, iconSize), iconbox), iconSize, opt.direction, instance, mode, state);
    }

    // the progress of a launch or an update, over all of it
    drawProgressOverlay(painter, opt, index.data(InstanceViewRoles::ProgressValueRole).toInt(),
                        index.data(InstanceViewRoles::ProgressMaximumRole).toInt());

    painter->restore();
}

QSize ListViewDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.features |= QStyleOptionViewItem::WrapText;
    opt.text = index.data().toString();
    opt.textElideMode = Qt::ElideRight;
    opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

    const int iconSize = iconSizeOf(option);
    const int width = InstanceView::itemWidthFor(iconSize);
    const int height = TOP_PADDING + iconSize + ICON_TEXT_GAP + viewItemTextSize(&opt, width - 2 * PADDING).height() + PADDING;
    return QSize(width, height);
}

class NoReturnTextEdit: public QTextEdit
{
    Q_OBJECT
public:
    explicit NoReturnTextEdit(QWidget *parent) : QTextEdit(parent)
    {
        setTextInteractionFlags(Qt::TextEditorInteraction);
        setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
        // the text where the name was, wrapped the same: no margin inside the field's frame
        // (the delegate places it), and as tall as the text, so nothing scrolls out of sight
        document()->setDocumentMargin(0);
        connect(document(), &QTextDocument::contentsChanged, this, &NoReturnTextEdit::fitHeight);
    }
    /// At least as tall as the name it covers (its lines are a little tighter than the item's)
    void setMinimumTextHeight(int height)
    {
        m_minimumTextHeight = height;
        fitHeight();
    }
    /// As tall as its text at its width; it may reach past the item, over the ones below
    void fitHeight()
    {
        document()->setTextWidth(viewport()->width());
        const int height = qMax(qCeil(document()->size().height()), m_minimumTextHeight) + 2 * frameWidth();
        if (height != this->height())
            resize(width(), height);
    }
    void resizeEvent(QResizeEvent *event) override
    {
        QTextEdit::resizeEvent(event);
        if (event->size().width() != event->oldSize().width())
            fitHeight();
    }
    bool event(QEvent * event) override
    {
        auto eventType = event->type();
        if(eventType == QEvent::KeyPress || eventType == QEvent::KeyRelease)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            auto key = keyEvent->key();
            if (key == Qt::Key_Return || key == Qt::Key_Enter)
            {
                emit editingDone();
                return true;
            }
            if(key == Qt::Key_Tab)
            {
                return true;
            }
        }
        return QTextEdit::event(event);
    }
signals:
    void editingDone();

private: /* data */
    int m_minimumTextHeight = 0;
};

void ListViewDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // the field's text exactly where the name is drawn (paint()), its frame around that; its
    // height follows the text (NoReturnTextEdit::fitHeight)
    auto field = qobject_cast<NoReturnTextEdit *>(editor);
    QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, option.widget) + 1;
    const int frame = field ? field->frameWidth() : 0;
    QRect textRect = option.rect.adjusted(textMargin + PADDING, TOP_PADDING + iconSizeOf(option) + ICON_TEXT_GAP, -textMargin - PADDING, 0);
    editor->setGeometry(textRect.left() - frame, textRect.top() - frame, textRect.width() + 2 * frame, editor->height());
    if (field)
    {
        // the name as drawn, so none of it shows below the field
        QStyleOptionViewItem opt = option;
        opt.text = index.data().toString();
        field->setMinimumTextHeight(viewItemTextSize(&opt, option.rect.width() - 2 * PADDING).height());
    }
}

void ListViewDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto text = index.data(Qt::EditRole).toString();
    QTextEdit * realeditor = qobject_cast<NoReturnTextEdit *>(editor);
    realeditor->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    realeditor->append(text);
    realeditor->selectAll();
    realeditor->document()->clearUndoRedoStacks();
}

void ListViewDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QTextEdit * realeditor = qobject_cast<NoReturnTextEdit *>(editor);
    QString text = realeditor->toPlainText();
    text.replace(QChar('\n'), QChar(' '));
    text = text.trimmed();
    if(text.size() != 0)
    {
        model->setData(index, text);
    }
}

QWidget * ListViewDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto editor = new NoReturnTextEdit(parent);
    editor->setFont(option.font);
    connect(editor, &NoReturnTextEdit::editingDone, this, &ListViewDelegate::editingDone);
    return editor;
}

void ListViewDelegate::editingDone()
{
    NoReturnTextEdit *editor = qobject_cast<NoReturnTextEdit *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

#include "InstanceDelegate.moc"
