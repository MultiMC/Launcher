#pragma once

#include "ITheme.h"

/// themes/<folder>/theme.json (colours) and themeStyle.css in the data folder, on top of the
/// chosen widget style. Written by hand; offered only when there is something custom in it.
class CustomTheme: public ITheme
{
public:
    explicit CustomTheme(QString folder);
    virtual ~CustomTheme() {}

    QString id() override;
    QString name() override;
    QPalette palette() override;
    QString styleSheet() override;
    QStringList searchPaths() override;

    bool isCustomized() const;

private: /* data */
    QPalette m_palette;
    QString m_styleSheet;
    QString m_name;
    QString m_id;
    bool m_customized = false;
};
