#pragma once

#include "settings/INISettingsObject.h"

class ApplicationSettings: public INISettingsObject {
    Q_OBJECT
public:
    ApplicationSettings(const QString & path, QObject * parent = nullptr);
    virtual ~ApplicationSettings() = default;
};
