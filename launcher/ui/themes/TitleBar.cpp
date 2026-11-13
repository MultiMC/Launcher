#include "TitleBar.h"

#include <QApplication>
#include <QEvent>
#include <QWidget>

TitleBar::Look TitleBarWatcher::s_look;

bool TitleBarWatcher::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type())
    {
    case QEvent::WinIdChange:
    case QEvent::Show:
    case QEvent::PaletteChange:
    // the active window's title bar can differ from the others' (Windows' accent title bars)
    case QEvent::ActivationChange:
    {
        auto widget = qobject_cast<QWidget *>(watched);
        // only windows that have a native window already: winId() would create one
        if (widget && widget->isWindow() && widget->testAttribute(Qt::WA_WState_Created))
            TitleBar::apply(widget, s_look);
        break;
    }
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

void TitleBarWatcher::setLook(const TitleBar::Look &look)
{
    s_look = look;
    refresh();
}

void TitleBarWatcher::refresh()
{
    for (auto widget : QApplication::topLevelWidgets())
    {
        if (widget->testAttribute(Qt::WA_WState_Created))
            TitleBar::apply(widget, s_look);
    }
}
