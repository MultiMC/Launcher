#include "SystemAppearance.h"

#include "MacOS.h"

#import <AppKit/AppKit.h>

// 10.14 APIs are used behind macOSAtLeast(10, 14) (see MacOS.h)
#pragma clang diagnostic ignored "-Wunguarded-availability"
#pragma clang diagnostic ignored "-Wunguarded-availability-new"

namespace {
enum class Mode
{
    Unknown, // before macOS 10.14: no dark mode
    Bright,
    Dark
};

// The system's setting, not the app's appearance (which Native may force): "Dark" when
// dark, missing when light (also with Auto, which sets it as it switches)
Mode readMode()
{
    if (macOSAtLeast(10, 14))
    {
        NSString *style = [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleInterfaceStyle"];
        return [style isEqualToString:@"Dark"] ? Mode::Dark : Mode::Bright;
    }
    return Mode::Unknown;
}

// The accent the user picked in System Preferences (blue for "multicolour")
SystemAppearance::Accent readAccent()
{
    if (macOSAtLeast(10, 14))
    {
        NSColor *accent = [[NSColor controlAccentColor] colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
        if (accent)
            return SystemAppearance::Accent::fromColor(QColor::fromRgbF(accent.redComponent, accent.greenComponent, accent.blueComponent));
    }
    return SystemAppearance::Accent();
}
}

// macOS posts distributed notifications when the appearance or the accent changes; only real
// differences count
class SystemAppearance::Private
{
public:
    explicit Private(SystemAppearance *q) : q(q), mode(readMode()), accent(readAccent())
    {
        auto center = [NSDistributedNotificationCenter defaultCenter];
        auto changed = ^(NSNotification *) {
            Mode nowMode = readMode();
            Accent nowAccent = readAccent();
            if (nowMode != mode || nowAccent != accent)
            {
                mode = nowMode;
                accent = nowAccent;
                emit this->q->changed();
            }
        };
        themeObserver = [center addObserverForName:@"AppleInterfaceThemeChangedNotification" object:nil
                                             queue:[NSOperationQueue mainQueue] usingBlock:changed];
        colorObserver = [center addObserverForName:@"AppleColorPreferencesChangedNotification" object:nil
                                             queue:[NSOperationQueue mainQueue] usingBlock:changed];
    }
    ~Private()
    {
        auto center = [NSDistributedNotificationCenter defaultCenter];
        [center removeObserver:themeObserver];
        [center removeObserver:colorObserver];
    }

    SystemAppearance *q;
    Mode mode;
    Accent accent;
    id themeObserver;
    id colorObserver;
};

SystemAppearance::SystemAppearance(QObject *parent) : QObject(parent), d(new Private(this))
{
}

SystemAppearance::~SystemAppearance()
{
    delete d;
}

bool SystemAppearance::canFollow() const
{
    return d->mode != Mode::Unknown;
}

bool SystemAppearance::isDark() const
{
    return d->mode == Mode::Dark;
}

SystemAppearance::Accent SystemAppearance::accent() const
{
    return d->accent;
}

// AppKit's semantic colours, resolved in the app's appearance (the system's in Fusion, where
// the app's isn't forced): what Mac windows, fields and text look like right now
QPalette SystemAppearance::palette() const
{
    if (d->mode == Mode::Unknown)
    {
        QPalette none;
        none.resolve(0);
        return none;
    }
    NSAppearance *saved = [NSAppearance currentAppearance];
    [NSAppearance setCurrentAppearance:[NSApp effectiveAppearance]];
    auto qt = [](NSColor *color) {
        NSColor *rgb = [color colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
        return rgb ? QColor::fromRgbF(rgb.redComponent, rgb.greenComponent, rgb.blueComponent, rgb.alphaComponent) : QColor();
    };
    const QColor window = qt([NSColor windowBackgroundColor]);
    const QColor base = qt([NSColor controlBackgroundColor]);
    const QColor text = qt([NSColor textColor]);
    NSArray<NSColor *> *alternating = [NSColor alternatingContentBackgroundColors];
    QColor alternateBase = alternating.count > 1 ? qt(alternating[1]) : base;
    [NSAppearance setCurrentAppearance:saved];
    // dark mode's alternate row is white at 5% opacity, drawn over the list: mixed over it here,
    // as a palette colour is drawn opaque (it came out solid white, seen 2026-11-07)
    QColor opaque = alternateBase;
    opaque.setAlphaF(1.0);
    alternateBase = Colors::mix(base, opaque, alternateBase.alphaF());

    // every role set: QPalette(button, window) derives the shading ones
    QPalette p(window, window);
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, alternateBase);
    p.setColor(QPalette::Button, window);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::ToolTipBase, base);
    p.setColor(QPalette::ToolTipText, text);
    return p;
}
