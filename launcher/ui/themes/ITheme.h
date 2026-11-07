#pragma once
#include <QString>
#include <QStringList>
#include <QPalette>
#include "Colors.h"
#include "TitleBar.h"

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
    /// With Fusion; the native style leaves title bars to the system
    virtual TitleBar::Style titleBarStyle()
    {
        return TitleBar::Style::Palette;
    }
};
