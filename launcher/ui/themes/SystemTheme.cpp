#include "SystemTheme.h"
#include "SystemAppearance.h"
#include "BrightTheme.h"
#include "DarkTheme.h"

SystemTheme::SystemTheme(SystemAppearance *system) : m_system(system)
{
}

QString SystemTheme::id()
{
    return "system";
}

QString SystemTheme::name()
{
    return QObject::tr("System");
}

QPalette SystemTheme::palette()
{
    QPalette p = m_system->palette();
    if (p.resolve() == 0)
    {
        p = m_system->isDark() ? DarkTheme::colors() : BrightTheme::colors();
    }
    return p;
}

Colors::Accent SystemTheme::accent()
{
    return m_system->accent();
}

// like other apps that follow the system: its colours, and on Windows 11 the accent on title
// bars when "Show accent color on title bars and window borders" is on (ours, which may be a
// custom one: TitleBar_win.cpp)
TitleBar::Style SystemTheme::titleBarStyle()
{
    return TitleBar::Style::Brightness;
}
