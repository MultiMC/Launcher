#pragma once

#include "BaseWizardPage.h"

class IconThemeSelectionWidget;
class QTabWidget;
class ThemeSelectionWidget;

/**
 * Choosing the theme and the icons, for anyone who hasn't chosen a theme yet (the Theme
 * setting is empty). Choices apply at once, so the wizard itself shows them.
 */
class ThemeWizardPage : public BaseWizardPage
{
    Q_OBJECT
public:
    explicit ThemeWizardPage(QWidget *parent = Q_NULLPTR);
    virtual ~ThemeWizardPage() {};

    bool validatePage() override;

protected:
    void retranslate() override;

private: /* data */
    QTabWidget *m_tabs = nullptr;
    ThemeSelectionWidget *m_themeWidget = nullptr;
    IconThemeSelectionWidget *m_iconsWidget = nullptr;
};
