#pragma once

#include "Colors.h"

#include <QObject>
#include <QPalette>

/// The system's bright/dark setting, colours and accent, and when they change.
/// Windows 10 1809+ (registry), macOS 10.14+ (AppKit), Linux (the desktop portal).
/// Qt 6.5+ has the bright/dark part for every platform (QStyleHints::colorScheme,
/// colorSchemeChanged), Qt 6.6+ the accent (QPalette::Accent): drop most of this class when
/// the Launcher gets there.
class SystemAppearance : public QObject
{
    Q_OBJECT
public:
    using Accent = Colors::Accent;

    explicit SystemAppearance(QObject *parent = nullptr);
    ~SystemAppearance();

    /// Whether the system has a bright/dark setting this can read
    bool canFollow() const;
    bool isDark() const;
    Accent accent() const;

    /// The system's own colours in its current mode, for the System theme on Fusion, so
    /// windows look like the system's (and match its title bars). Every role set (accent and
    /// disabled colours are added by the theme). Without any: a palette with no roles set
    /// (resolve() == 0), and the System theme uses our Bright or Dark.
    QPalette palette() const;

signals:
    void changed();

private:
    class Private;
    Private *d;
};
