#pragma once

#include <QWidget>

#include "ui/pages/BasePage.h"
#include <Application.h>

class QGroupBox;
class IconThemeSelectionWidget;
class ThemeSelectionWidget;

/**
 * How the launcher looks: the theme, accent and style (ThemeSelectionWidget), and the icons
 * (IconThemeSelectionWidget). Choices apply at once.
 */
class AppearancePage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit AppearancePage(QWidget *parent = 0);
    virtual ~AppearancePage() {};

    QString displayName() const override
    {
        return tr("Appearance");
    }
    QIcon icon() const override
    {
        return APPLICATION->getThemedIcon("resourcepacks");
    }
    QString id() const override
    {
        return "appearance-settings";
    }
    QString helpPage() const override
    {
        return "Launcher-settings";
    }
    void changeEvent(QEvent *) override;

private:
    void loadSettings();
    void retranslate();

private: /* data */
    QGroupBox *m_themeBox = nullptr;
    ThemeSelectionWidget *m_themeWidget = nullptr;
    QGroupBox *m_iconsBox = nullptr;
    IconThemeSelectionWidget *m_iconsWidget = nullptr;
};
