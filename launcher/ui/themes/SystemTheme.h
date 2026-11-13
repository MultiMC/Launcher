#pragma once

#include "ITheme.h"

class SystemAppearance;

/// Bright or Dark with the system, in the system's own colours where we know them (Windows'
/// dark mode, macOS), and the system's accent as its own
class SystemTheme: public ITheme
{
public:
    explicit SystemTheme(SystemAppearance *system);
    virtual ~SystemTheme() {}

    QString id() override;
    QString name() override;
    QPalette palette() override;
    Colors::Accent accent() override;

private: /* data */
    SystemAppearance *m_system = nullptr;
};
