#pragma once
#include <QString>
#include <QStringList>
#include <QPalette>
#include <QMap>
#include "Colors.h"
#include "TitleBar.h"

/// Where a theme's accent comes from
enum class AccentMode
{
    System, // the system's, followed live; the theme's own where the system has none
    Custom  // the theme's own
};

/// What a theme's title bars look like (with Fusion)
enum class TitleBarMode
{
    System, // the system's: only dark or light from us (Windows draws its accent, if set so)
    Window, // the window's colours (Window, WindowText)
    Accent, // the active window's in the accent, the others the window's (Windows 11)
    Custom  // colours of the theme's own (titleBarColor, titleBarTextColor)
};

/// A theme: colours for the widget style, which ThemeManager finishes (the accent, then the
/// disabled colours) and applies. Design/Theming.md in the MultiMC repository.
class ITheme
{
public:
    virtual ~ITheme() {}
    virtual QString id() = 0;
    virtual QString name() = 0;
    /// Every role set (QPalette(button, window) derives the shading ones): roles left out
    /// would come from the platform palette, which is the desktop's colour scheme on KDE
    virtual QPalette palette() = 0;
    /// The theme's own accent, used when the accent setting says "the theme's"; invalid
    /// means the palette's own Highlight and Link
    virtual Colors::Accent accent()
    {
        return {};
    }
    virtual QString styleSheet()
    {
        return {};
    }
    virtual QStringList searchPaths()
    {
        return {};
    }
    /// Colours for log levels, by name (Launcher, Message, Debug, Warning, Error, Fatal,
    /// FatalHighlight); the ones missing are the log's own
    virtual QMap<QString, QColor> logColors()
    {
        return {};
    }
    /// The accent: the system's (where it has one; else accent()), or the theme's own
    virtual AccentMode accentMode()
    {
        return AccentMode::System;
    }
    /// With Fusion; the native style leaves title bars to the system
    virtual TitleBarMode titleBarMode()
    {
        return TitleBarMode::System;
    }
    /// The title bar's colours with TitleBarMode::Custom
    virtual QColor titleBarColor()
    {
        return {};
    }
    virtual QColor titleBarTextColor()
    {
        return {};
    }
};
