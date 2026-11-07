#include "TitleBar.h"
#include "Colors.h"

#include <QColor>
#include <QPalette>
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

void TitleBar::apply(QWidget *window, Style style)
{
    if (!window || !window->isWindow() || !dwmSetWindowAttribute())
        return;
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    const QPalette palette = window->palette();
    const QColor caption = palette.color(QPalette::Window);
    const QColor text = palette.color(QPalette::WindowText);
    const bool colored = style == Style::Palette;
    // Windows 10 only has the dark/light switch; on 11 it also picks the caption buttons' look
    const DWORD dark = style != Style::Default && Colors::isDark(caption);
    // a little darker than the caption, so overlapping windows don't merge
    const QColor border = caption.darker(140);

    set(hwnd, windowsBuild() >= 18985 ? DWMWA_USE_IMMERSIVE_DARK_MODE : DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, dark);
    // also with the system's colours: back from the palette's
    if (windowsBuild() >= 22000)
    {
        set(hwnd, DWMWA_CAPTION_COLOR, colored ? colorRef(caption) : DWMWA_COLOR_DEFAULT);
        set(hwnd, DWMWA_TEXT_COLOR, colored ? colorRef(text) : DWMWA_COLOR_DEFAULT);
        set(hwnd, DWMWA_BORDER_COLOR, colored ? colorRef(border) : DWMWA_COLOR_DEFAULT);
    }
    // Windows 10 doesn't redraw the title bar until something makes it; this nudges it
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

// Windows has no application-wide appearance: windows are set up one by one
bool TitleBar::setApplicationAppearance(Appearance)
{
    return false;
}
