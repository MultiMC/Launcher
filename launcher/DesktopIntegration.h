#pragma once

#include <QString>

/**
 * The Launcher's own entry in the desktop's applications menu, and its icon, for copies that
 * aren't installed as a package (Linux; elsewhere nothing). Docks and task managers also take
 * the window icon from this entry: they match windows to it by the Wayland app id and the X11
 * window class (org.multimc.MultiMC, MultiMC), which is why it carries that name.
 *
 * The entry and icon are the user's (XDG_DATA_HOME: applications/org.multimc.MultiMC.desktop,
 * icons/hicolor/scalable/apps/org.multimc.MultiMC.svg) and name the copy that wrote them
 * (X-MultiMC-Root). One copy owns the entry: others leave it alone until that copy is gone
 * (its start script missing), or the user switches it in the settings. Its TryExec hides it
 * from the menu once the copy is deleted.
 *
 * Nothing is written for a copy in a temporary folder or without its start script, or when a
 * package provides the entry (system-wide org.multimc.MultiMC.desktop, the Packaging repo's):
 * then an entry a copy wrote is removed, so it doesn't hide the package's.
 */
namespace DesktopIntegration
{
enum class State
{
    Unsupported, // not Linux: nothing to manage
    Unavailable, // a copy that gets none (see why)
    Packaged,    // a package provides the entry
    Ours,        // the entry is this copy's
    Other        // another copy's (other is its folder), or one the Launcher didn't write
};

struct Status
{
    State state = State::Unavailable;
    QString why;
    QString other;
};

/// root: this copy's folder (the one with the MultiMC start script)
Status status(const QString &root);

/// At start: this copy's entry written or refreshed, shown in the menu or not; an entry whose
/// copy is gone taken over; nothing else touched
void update(const QString &root, bool shown);

/// The entry made this copy's (the settings' "Use this copy"); false where that can't be
bool takeOver(const QString &root, bool shown);
}
