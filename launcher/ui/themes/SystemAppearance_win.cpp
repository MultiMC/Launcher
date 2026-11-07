#include "SystemAppearance.h"

#include <QAbstractNativeEventFilter>
#include <QCoreApplication>

#include <windows.h>

namespace {
enum class Mode
{
    Unknown, // no such setting: before Windows 10 1809
    Bright,
    Dark
};

// Not in the toolchain's headers (Vista+)
const UINT WM_DWMCOLORIZATIONCOLORCHANGED_ = 0x0320;

// RegGetValue would need Vista
bool readValue(const wchar_t *path, const wchar_t *name, DWORD wantedType, void *data, DWORD size)
{
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return false;
    DWORD type = 0, got = size;
    LONG result = RegQueryValueExW(key, name, nullptr, &type, static_cast<BYTE *>(data), &got);
    RegCloseKey(key);
    return result == ERROR_SUCCESS && type == wantedType && got == size;
}

// "Choose your default app mode"
Mode readMode()
{
    DWORD light = 0;
    if (!readValue(L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", REG_DWORD,
                   &light, sizeof(light)))
        return Mode::Unknown;
    return light ? Mode::Bright : Mode::Dark;
}

// Windows 10+ keeps the accent with its shades (the ones UISettings.GetColorValue gives) in
// AccentPalette: 8 times R, G, B, A, from Light3 to Dark3, then one Windows doesn't use.
// Without that, DWM's AccentColor (0xAABBGGRR) and shades of our own.
SystemAppearance::Accent readAccent()
{
    SystemAppearance::Accent accent;
    BYTE palette[32];
    if (readValue(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent", L"AccentPalette", REG_BINARY, palette,
                  sizeof(palette)))
    {
        auto shade = [&palette](int i) { return QColor(palette[i * 4], palette[i * 4 + 1], palette[i * 4 + 2]); };
        accent.light3 = shade(0);
        accent.light2 = shade(1);
        accent.light1 = shade(2);
        accent.color = shade(3);
        accent.dark1 = shade(4);
        accent.dark2 = shade(5);
        accent.dark3 = shade(6);
        return accent;
    }
    DWORD abgr = 0;
    if (readValue(L"Software\\Microsoft\\Windows\\DWM", L"AccentColor", REG_DWORD, &abgr, sizeof(abgr)))
        return SystemAppearance::Accent::fromColor(QColor(abgr & 0xFF, (abgr >> 8) & 0xFF, (abgr >> 16) & 0xFF));
    return accent;
}
}

// Windows broadcasts WM_SETTINGCHANGE "ImmersiveColorSet" to top-level windows when the
// mode or the accent changes (and on other colour changes: only real differences count);
// DWM also sends WM_DWMCOLORIZATIONCOLORCHANGED for the accent
class SystemAppearance::Private : public QAbstractNativeEventFilter
{
public:
    explicit Private(SystemAppearance *q) : q(q), mode(readMode()), accent(readAccent())
    {
        QCoreApplication::instance()->installNativeEventFilter(this);
    }
    ~Private()
    {
        QCoreApplication::instance()->removeNativeEventFilter(this);
    }

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *) override
    {
        if (eventType != "windows_generic_MSG")
            return false;
        auto msg = static_cast<MSG *>(message);
        bool settingChange = msg->message == WM_SETTINGCHANGE && msg->lParam
            && wcscmp(reinterpret_cast<const wchar_t *>(msg->lParam), L"ImmersiveColorSet") == 0;
        if (!settingChange && msg->message != WM_DWMCOLORIZATIONCOLORCHANGED_)
            return false;
        Mode nowMode = readMode();
        Accent nowAccent = readAccent();
        if (nowMode != mode || nowAccent != accent)
        {
            mode = nowMode;
            accent = nowAccent;
            emit q->changed();
        }
        return false;
    }

    SystemAppearance *q;
    Mode mode;
    Accent accent;
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

// Qt 5.6 only knows Windows' classic system colours, light even in dark mode, so these are
// Windows 11's own (its Settings app and the WinUI defaults), which also match the system's
// default title bars (#202020). Dark mode only: in light mode the System theme takes our
// Bright, which is Windows 11's light colours already (BrightTheme.cpp).
QPalette SystemAppearance::palette() const
{
    if (d->mode != Mode::Dark)
    {
        QPalette none;
        none.resolve(0);
        return none;
    }
    const QColor window(0x20, 0x20, 0x20), button(0x2d, 0x2d, 0x2d), base(0x2b, 0x2b, 0x2b), text(Qt::white);
    // every role set: QPalette(button, window) derives the shading ones
    QPalette p(button, window);
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, QColor(0x32, 0x32, 0x32));
    p.setColor(QPalette::Button, button);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::ToolTipBase, base);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::BrightText, QColor(0xff, 0x99, 0xa4));
    return p;
}
