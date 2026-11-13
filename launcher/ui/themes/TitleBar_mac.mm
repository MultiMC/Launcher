#include "TitleBar.h"
#include "Colors.h"

#include <QPalette>
#include <QWidget>

#include "MacOS.h"

#import <AppKit/AppKit.h>

// 10.14 APIs are used behind macOSAtLeast(10, 14) (see MacOS.h)
#pragma clang diagnostic ignored "-Wunguarded-availability"
#pragma clang diagnostic ignored "-Wunguarded-availability-new"

// Dark aqua exists from macOS 10.14; on 10.13 title bars only get the palette's colour, and
// the application's appearance stays the system's.
namespace {
NSColor *nsColor(const QColor &color)
{
    return [NSColor colorWithSRGBRed:color.redF() green:color.greenF() blue:color.blueF() alpha:1.0];
}
}

void TitleBar::apply(QWidget *window, const Look &look)
{
    // the accent look isn't done here yet: the plain one
    const Style style = look.style;
    if (!window || !window->isWindow())
        return;
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    NSWindow *nsWindow = view.window;
    if (!nsWindow)
        return;
    const QColor caption = look.caption.isValid() ? look.caption : window->palette().color(QPalette::Window);
    const bool colored = style == Style::Palette;

    if (macOSAtLeast(10, 14))
    {
        // nil: the application's appearance (setApplicationAppearance), else the system's;
        // otherwise dark or light like the palette, which also gives the title text and
        // buttons the right look
        NSAppearance *appearance = nil;
        if (style != Style::Default)
            appearance = [NSAppearance appearanceNamed:Colors::isDark(caption) ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua];
        nsWindow.appearance = appearance;
    }
    // The palette's colour: a transparent title bar shows the window's background colour
    nsWindow.titlebarAppearsTransparent = colored;
    nsWindow.backgroundColor = colored ? nsColor(caption) : [NSColor windowBackgroundColor];
}

bool TitleBar::setApplicationAppearance(Appearance appearance)
{
    if (!macOSAtLeast(10, 14))
        return false;
    switch (appearance)
    {
    case Appearance::System:
        NSApp.appearance = nil;
        break;
    case Appearance::Bright:
        NSApp.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
        break;
    case Appearance::Dark:
        NSApp.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
        break;
    }
    return true;
}
