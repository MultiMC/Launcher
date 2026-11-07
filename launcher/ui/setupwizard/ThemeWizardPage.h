#pragma once

#include "BaseWizardPage.h"

class IconThemeSelectionWidget;
class QGroupBox;
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
    QGroupBox *m_themeBox = nullptr;
    ThemeSelectionWidget *m_themeWidget = nullptr;
    QGroupBox *m_iconsBox = nullptr;
    IconThemeSelectionWidget *m_iconsWidget = nullptr;
};
