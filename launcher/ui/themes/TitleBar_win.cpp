#include "TitleBar.h"
#include "Colors.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
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

}

void TitleBar::apply(QWidget *window, const Look &look)
{
    const Style style = look.style;
    if (!window || !window->isWindow() || !dwmSetWindowAttribute())
        return;
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    const QPalette palette = window->palette();
    // The plain look: the theme's title bar colours, else the window's
    const QColor plainCaption = look.caption.isValid() ? look.caption : palette.color(QPalette::Window);
    const QColor plainText = look.text.isValid() ? look.text : palette.color(QPalette::WindowText);
    // As Windows does with its accent title bars: the active window's only, the others plain.
    // From the application's palette: a theme's stylesheet can give the window's own palette
    // other selection colours (Borest: a translucent white, white text on it).
    const bool accent = windowsBuild() >= 22000 && style == Style::Palette && look.accent && window->isActiveWindow();
    const QPalette theme = QApplication::palette();
    const QColor accentCaption = theme.color(QPalette::Active, QPalette::Highlight);

    // Windows 10 only has the dark/light switch; on 11 it also picks the caption buttons' look,
    // so it goes with the caption shown
    const DWORD dark = style != Style::Default && Colors::isDark(accent ? accentCaption : plainCaption);
    // Windows 11: the colours, or DWMWA_COLOR_DEFAULT for the system's. The border is the
    // system's unless it's the accent: its grey edge, a little lighter than a dark caption,
    // keeps windows apart; a darker one of ours vanished against dark windows behind.
    DWORD caption = DWMWA_COLOR_DEFAULT, text = DWMWA_COLOR_DEFAULT, border = DWMWA_COLOR_DEFAULT;
    if (accent)
    {
        caption = border = colorRef(accentCaption);
        text = colorRef(theme.color(QPalette::Active, QPalette::HighlightedText));
    }
    else if (windowsBuild() >= 22000 && style == Style::Palette)
    {
        caption = colorRef(plainCaption);
        text = colorRef(plainText);
    }

    set(hwnd, windowsBuild() >= 18985 ? DWMWA_USE_IMMERSIVE_DARK_MODE : DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, dark);
    // also with the system's colours: back from the palette's. Set every time: cheap, and
    // Windows may have put its own back (its accent title bar setting changed)
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
