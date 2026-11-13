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
