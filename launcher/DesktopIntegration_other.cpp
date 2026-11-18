/* Copyright 2026 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "DesktopIntegration.h"

// Windows and macOS: the system takes the icon from the executable or the app bundle, and
// installers or the user make shortcuts

DesktopIntegration::Status DesktopIntegration::status(const QString &)
{
    Status status;
    status.state = State::Unsupported;
    return status;
}

void DesktopIntegration::update(const QString &, bool)
{
}

bool DesktopIntegration::takeOver(const QString &, bool)
{
    return false;
}
