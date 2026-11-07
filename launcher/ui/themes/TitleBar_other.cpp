#include "TitleBar.h"

// Title bars are the window manager's (X11) or the compositor's (Wayland)
void TitleBar::apply(QWidget *, Style)
{
}

bool TitleBar::setApplicationAppearance(Appearance)
{
    return false;
}
