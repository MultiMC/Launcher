#pragma once

#include <QList>
#include <QWidget>

class QAction;
class QMenu;
class QToolBar;
class QToolButton;

/**
 * The selected instance's panel, the one item of the instance tool bar (#instanceToolBar): a
 * header (the icon and the name) on top, then a button for each action, with separators. All
 * at their own heights: when they don't all fit, the actions at the end go into the More
 * button's menu (#instancePanelMore), and nothing is squeezed.
 *
 * Laid out here rather than by the tool bar: Qt 5's tool bar layout makes room for its
 * extension button by dropping one item, and when that's a separator, the rest still don't
 * fit; it then places the items as if the biggest (the icon) had shrunk below its minimum,
 * which it doesn't, and the name slid into the icon (Design/Theming.md, 2026-11-17).
 *
 * For stylesheets: #instancePanel, its buttons QToolButton#<action's object name>, the header's
 * #changeIconButton and #renameButton, separators drawn as the tool bar's (QToolBar::separator),
 * and QToolButton#instancePanelMore.
 */
class InstancePanel : public QWidget
{
    Q_OBJECT
public:
    explicit InstancePanel(QToolBar *toolBar);
    virtual ~InstancePanel() {};

    /// Always shown, at the top, in the order added
    void addHeaderWidget(QWidget *widget);
    /// A button for the action, named after it; it follows the action (text, menu, enabled)
    QToolButton *addActionButton(QAction *action);
    void addSeparator();
    /// The button made for an action, or null
    QToolButton *buttonForAction(QAction *action) const;
    /// The actions and separators in order, as a menu would list them
    QList<QAction *> menuActions() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Entry
    {
        Entry(QAction *action = nullptr, QWidget *widget = nullptr) : action(action), widget(widget) {}
        QAction *action; // a separator action for a separator
        QWidget *widget; // its button, or its separator
    };

    void relayout();
    int itemHeight(QWidget *widget) const;
    QRect rowRect(QWidget *widget, int y, int height) const;
    int spacing() const;
    void applyToolBarLook(QToolButton *button) const;

private: /* data */
    QToolBar *m_toolBar = nullptr;
    QList<QWidget *> m_header;
    QList<Entry> m_entries;
    QToolButton *m_more = nullptr;
    QMenu *m_moreMenu = nullptr;
    bool m_layingOut = false;
};
