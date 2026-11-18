/* Copyright 2026 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "TitleBar.h"

// Title bars are the window manager's (X11) or the compositor's (Wayland)
void TitleBar::apply(QWidget *, Style)
{
}

bool TitleBar::setApplicationAppearance(Appearance)
{
    return false;
}
