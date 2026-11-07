#pragma once

#include "ITheme.h"

class BrightTheme: public ITheme
{
public:
    virtual ~BrightTheme() {}

    QString id() override;
    QString name() override;
    QPalette palette() override;

    static QPalette colors();
};
