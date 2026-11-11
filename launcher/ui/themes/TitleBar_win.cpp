#include "TitleBar.h"
#include "Colors.h"

#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QDebug>
#include <QPalette>
#include <QTimer>
#include <QVariant>
#include <QWidget>

#include <sys.h>

#include <windows.h>

// Everything is looked up at run time: dwmapi.dll doesn't exist on XP, and the attributes
// below are newer than the toolchain's headers.
// Qt 6's windows plugin sets the dark/light title bar itself (to verify), but not custom
// caption and border colours: those stay ours.
namespace {
// Windows 10 1809+ (19 before build 18985, 20 after): dark title bar, system colours
const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_OLD = 19;
const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
// Windows 11 (build 22000+): any colour; DWMWA_COLOR_DEFAULT goes back to the system's
const DWORD DWMWA_BORDER_COLOR = 34;
const DWORD DWMWA_CAPTION_COLOR = 35;
const DWORD DWMWA_TEXT_COLOR = 36;
const DWORD DWMWA_COLOR_DEFAULT = 0xFFFFFFFF;

typedef HRESULT (WINAPI *DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);

DwmSetWindowAttributeFn dwmSetWindowAttribute()
{
    static DwmSetWindowAttributeFn fn = []() -> DwmSetWindowAttributeFn {
        HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
        if (!dwm)
            return nullptr;
        return reinterpret_cast<DwmSetWindowAttributeFn>(GetProcAddress(dwm, "DwmSetWindowAttribute"));
    }();
    return fn;
}

// the real build: our manifest declares Windows 10 and 11, so GetVersionEx doesn't lie
int windowsBuild()
{
    static int build = Sys::getKernelInfo().kernelPatch;
    return build;
}

DWORD colorRef(const QColor &color)
{
    return RGB(color.red(), color.green(), color.blue());
}

void set(HWND hwnd, DWORD attribute, DWORD value)
{
    dwmSetWindowAttribute()(hwnd, attribute, &value, sizeof(value));
}

// "Show accent color on title bars and window borders"
bool readAccentOnTitleBars()
{
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\DWM", 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return false;
    DWORD type = 0, value = 0, size = sizeof(value);
    LONG result = RegQueryValueExW(key, L"ColorPrevalence", nullptr, &type, reinterpret_cast<BYTE *>(&value), &size);
    RegCloseKey(key);
    return result == ERROR_SUCCESS && type == REG_DWORD && value != 0;
}

// Not in the toolchain's headers (Vista+)
const UINT WM_DWMCOLORIZATIONCOLORCHANGED_ = 0x0320;

// The setting, kept up to date: switching it sends WM_DWMCOLORIZATIONCOLORCHANGED and
// WM_SETTINGCHANGE "ImmersiveColorSet" to the top-level windows, then they're set up again
class AccentOnTitleBars : public QAbstractNativeEventFilter
{
public:
    AccentOnTitleBars() : m_on(readAccentOnTitleBars())
    {
        QCoreApplication::instance()->installNativeEventFilter(this);
    }

    bool on() const
    {
        return m_on;
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
        const bool now = readAccentOnTitleBars();
        if (now != m_on)
        {
            m_on = now;
            qDebug() << "Accent on title bars:" << m_on;
            TitleBarWatcher::refresh();
            // and once more when Windows is done with its own title bars, which can take
            // their colours back (the text colour of ours) after this message
            QTimer::singleShot(500, []() { TitleBarWatcher::refresh(); });
        }
        return false;
    }

private:
    bool m_on;
};

bool accentOnTitleBars()
{
    // made with the first window: there is an application by then
    static AccentOnTitleBars *setting = new AccentOnTitleBars();
    return setting->on();
}
}

void TitleBar::apply(QWidget *window, Style style)
{
    if (!window || !window->isWindow() || !dwmSetWindowAttribute())
        return;
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    const QPalette palette = window->palette();
    const QColor windowColor = palette.color(QPalette::Window);
    // Windows 10 only has the dark/light switch; on 11 it also picks the caption buttons' look
    const DWORD dark = style != Style::Default && Colors::isDark(windowColor);
    // Windows 11: the colours, or DWMWA_COLOR_DEFAULT for the system's. The border is the
    // system's unless it's the accent: its grey edge, a little lighter than a dark caption,
    // keeps windows apart; a darker one of ours vanished against dark windows behind.
    DWORD caption = DWMWA_COLOR_DEFAULT, text = DWMWA_COLOR_DEFAULT, border = DWMWA_COLOR_DEFAULT;
    if (windowsBuild() >= 22000 && style != Style::Default)
    {
        // as Windows does with its accent title bars: the active window's only, the others
        // as if the setting were off. Our accent, not the system's: it may be a custom one.
        // The application's palette: a theme's stylesheet can give the window's own palette
        // other selection colours (Borest: a translucent white, white text on it).
        if (accentOnTitleBars() && window->isActiveWindow())
        {
            const QPalette theme = QApplication::palette();
            caption = border = colorRef(theme.color(QPalette::Active, QPalette::Highlight));
            text = colorRef(theme.color(QPalette::Active, QPalette::HighlightedText));
        }
        else if (style == Style::Palette)
        {
            caption = colorRef(windowColor);
            text = colorRef(palette.color(QPalette::WindowText));
        }
    }

    set(hwnd, windowsBuild() >= 18985 ? DWMWA_USE_IMMERSIVE_DARK_MODE : DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, dark);
    // also with the system's colours: back from the palette's. Set every time: Windows sets
    // its own again when the accent title bar setting changes, maybe after we did.
    if (windowsBuild() >= 22000)
    {
        set(hwnd, DWMWA_CAPTION_COLOR, caption);
        set(hwnd, DWMWA_TEXT_COLOR, text);
        set(hwnd, DWMWA_BORDER_COLOR, border);
    }

    // Redraw only when something differs: this runs on every activation change, and the
    // redraws below aren't free. A new native window starts over.
    const QString applied = QString("%1 %2 %3 %4 %5").arg(quintptr(hwnd)).arg(dark).arg(caption).arg(text).arg(border);
    if (window->property("titleBarApplied").toString() == applied)
        return;
    window->setProperty("titleBarApplied", applied);

    // Windows 10 doesn't redraw the title bar until something makes it; this nudges it
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    // ...but not the active window's: that one is only drawn again when activation changes
    // (another window opening over it). Telling it it's inactive, then what it really is,
    // draws its frame twice without changing which window is active. Windows 11 redraws
    // on its own.
    if (windowsBuild() < 22000)
    {
        const bool active = GetActiveWindow() == hwnd;
        SendMessageW(hwnd, WM_NCACTIVATE, !active, 0);
        SendMessageW(hwnd, WM_NCACTIVATE, active, 0);
    }
}

// Windows has no application-wide appearance: windows are set up one by one
bool TitleBar::setApplicationAppearance(Appearance)
{
    return false;
}
