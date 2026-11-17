#include "InstancePanel.h"

#include <QAction>
#include <QEvent>
#include <QMenu>
#include <QPainter>
#include <QResizeEvent>
#include <QStyle>
#include <QStyleOption>
#include <QToolBar>
#include <QToolButton>

namespace {
/**
 * A separator between the panel's buttons, drawn as the tool bar draws its own: with the tool
 * bar as the widget, so stylesheets' QToolBar::separator rules apply to it
 */
class PanelSeparator : public QWidget
{
public:
    PanelSeparator(QToolBar *toolBar, QWidget *parent) : QWidget(parent), m_toolBar(toolBar)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    QSize sizeHint() const override
    {
        const int extent = m_toolBar->style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent, nullptr, m_toolBar);
        return QSize(extent, extent);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        // the separator line, across the panel (it's a vertical tool bar: no State_Horizontal)
        QPainter p(this);
        QStyleOption option;
        option.initFrom(this);
        m_toolBar->style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &option, &p, m_toolBar);
    }

private: /* data */
    QToolBar *m_toolBar;
};
}

InstancePanel::InstancePanel(QToolBar *toolBar) : QWidget(toolBar), m_toolBar(toolBar)
{
    setObjectName(QStringLiteral("instancePanel"));
    // all the tool bar's length: the tool bar gives an item that expands the room it has
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    m_moreMenu = new QMenu(this);
    m_more = new QToolButton(this);
    m_more->setObjectName(QStringLiteral("instancePanelMore"));
    // the menu opened on a click rather than set on the button: no menu arrow on it, like the
    // tool bar's own extension button
    connect(m_more, &QToolButton::clicked, this, [this]() {
        m_moreMenu->exec(m_more->mapToGlobal(QPoint(0, m_more->height())));
        m_more->setDown(false);
    });
    m_more->setToolTip(tr("More"));
    applyToolBarLook(m_more);
    // the tool bar's own extension button's arrow
    m_more->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_more->setIcon(style()->standardIcon(QStyle::SP_ToolBarVerticalExtensionButton, nullptr, m_toolBar));
    m_more->hide();

    connect(m_toolBar, &QToolBar::toolButtonStyleChanged, this, [this]() {
        for (auto &entry : m_entries)
        {
            if (auto button = qobject_cast<QToolButton *>(entry.widget))
                applyToolBarLook(button);
        }
        relayout();
    });
}

void InstancePanel::applyToolBarLook(QToolButton *button) const
{
    // as the tool bar makes its own buttons
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setToolButtonStyle(m_toolBar->toolButtonStyle());
    button->setIconSize(m_toolBar->iconSize());
}

void InstancePanel::addHeaderWidget(QWidget *widget)
{
    widget->setParent(this);
    // flat until hovered, as the tool bar had them
    if (auto button = qobject_cast<QToolButton *>(widget))
        button->setAutoRaise(true);
    m_header.append(widget);
    relayout();
}

QToolButton *InstancePanel::addActionButton(QAction *action)
{
    auto button = new QToolButton(this);
    button->setDefaultAction(action);
    // so stylesheets can pick it out: QToolButton#actionLaunchInstance
    button->setObjectName(action->objectName());
    applyToolBarLook(button);
    m_entries.append(Entry(action, button));
    relayout();
    return button;
}

void InstancePanel::addSeparator()
{
    auto action = new QAction(this);
    action->setSeparator(true);
    m_entries.append(Entry(action, new PanelSeparator(m_toolBar, this)));
    relayout();
}

QToolButton *InstancePanel::buttonForAction(QAction *action) const
{
    for (auto &entry : m_entries)
    {
        if (entry.action == action)
            return qobject_cast<QToolButton *>(entry.widget);
    }
    return nullptr;
}

QList<QAction *> InstancePanel::menuActions() const
{
    QList<QAction *> out;
    for (auto &entry : m_entries)
        out.append(entry.action);
    return out;
}

int InstancePanel::spacing() const
{
    return m_toolBar->style()->pixelMetric(QStyle::PM_ToolBarItemSpacing, nullptr, m_toolBar);
}

// What the widget needs: its size hint, or more where its minimum says so (the name's height
// follows its width: LabeledToolButton)
int InstancePanel::itemHeight(QWidget *widget) const
{
    return qMax(widget->sizeHint().height(), widget->minimumHeight());
}

QSize InstancePanel::sizeHint() const
{
    int width = 0;
    int height = 0;
    auto add = [&](QWidget *widget) {
        width = qMax(width, widget->sizeHint().width());
        height += (height ? spacing() : 0) + itemHeight(widget);
    };
    for (auto widget : m_header)
        add(widget);
    for (auto &entry : m_entries)
        add(entry.widget);
    return QSize(width, height);
}

QSize InstancePanel::minimumSizeHint() const
{
    // the header and the More button: the rest can go into its menu
    int width = m_more->sizeHint().width();
    int height = itemHeight(m_more);
    for (auto widget : m_header)
    {
        width = qMax(width, widget->minimumSizeHint().width());
        height += spacing() + itemHeight(widget);
    }
    return QSize(width, height);
}

bool InstancePanel::event(QEvent *event)
{
    // a button's or the name's size changed (a new text, a longer name wrapping into more lines)
    if (event->type() == QEvent::LayoutRequest)
    {
        updateGeometry();
        relayout();
    }
    return QWidget::event(event);
}

void InstancePanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

// Where a widget goes in a row of the panel: the whole width for one that expands sideways (the
// header, separators), else its own width, centred, as the tool bar placed its buttons
QRect InstancePanel::rowRect(QWidget *widget, int y, int height) const
{
    const int width = this->width();
    if (widget->sizePolicy().horizontalPolicy() & QSizePolicy::ExpandFlag)
        return QRect(0, y, width, height);
    const int own = qMin(widget->sizeHint().width(), width);
    return QRect((width - own) / 2, y, own, height);
}

// Top to bottom at their own heights: the header, then the buttons while they fit. When they
// don't all fit, the More button takes the last place and the rest go into its menu; a separator
// right before it isn't shown, and none starts the menu.
void InstancePanel::relayout()
{
    if (m_layingOut)
        return;
    m_layingOut = true;
    const int width = this->width();
    const int gap = spacing();
    int y = 0;
    for (auto widget : m_header)
    {
        // the name's height depends on the width: give it the width first
        widget->resize(width, widget->height());
        const int height = itemHeight(widget);
        widget->setGeometry(rowRect(widget, y, height));
        widget->show();
        y += height + gap;
    }

    int needed = 0;
    for (auto &entry : m_entries)
        needed += itemHeight(entry.widget) + gap;
    const bool overflow = y + needed - gap > this->height();
    const int limit = this->height() - (overflow ? itemHeight(m_more) + gap : 0);

    int shown = 0;
    for (; shown < m_entries.size(); shown++)
    {
        QWidget *widget = m_entries[shown].widget;
        const int height = itemHeight(widget);
        if (overflow && y + height > limit)
            break;
        widget->setGeometry(rowRect(widget, y, height));
        widget->show();
        y += height + gap;
    }
    // no separator as the last one shown before More
    int lastShown = shown - 1;
    while (overflow && lastShown >= 0 && m_entries[lastShown].action->isSeparator())
    {
        m_entries[lastShown].widget->hide();
        y -= itemHeight(m_entries[lastShown].widget) + gap;
        lastShown--;
    }

    m_moreMenu->clear();
    for (int i = shown; i < m_entries.size(); i++)
    {
        m_entries[i].widget->hide();
        // the menu starts with an action, not a separator
        if (m_entries[i].action->isSeparator() && m_moreMenu->actions().isEmpty())
            continue;
        m_moreMenu->addAction(m_entries[i].action);
    }
    if (overflow)
    {
        m_more->setGeometry(rowRect(m_more, y, itemHeight(m_more)));
        m_more->show();
    }
    else
    {
        m_more->hide();
    }
    m_layingOut = false;
}
