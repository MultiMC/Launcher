#pragma once

#include <QWidget>

#include "ui/pages/BasePage.h"
#include <Application.h>

class IconThemeSelectionWidget;
class QFontComboBox;
class QLabel;
class QSpinBox;
class QTabWidget;
class QTextEdit;
class ThemeSelectionWidget;

/**
 * How the launcher looks, in tabs: the theme, accent and style (ThemeSelectionWidget), the
 * icons (IconThemeSelectionWidget), and the console's font, with a preview in the theme's log
 * colours. Choices apply at once.
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
    QWidget *makeConsoleTab();
    void consoleFontChanged();
    void updateConsolePreview();
    void loadSettings();
    void retranslate();

private: /* data */
    QTabWidget *m_tabs = nullptr;
    ThemeSelectionWidget *m_themeWidget = nullptr;
    IconThemeSelectionWidget *m_iconsWidget = nullptr;
    QWidget *m_consoleTab = nullptr;
    QLabel *m_fontLabel = nullptr;
    QFontComboBox *m_consoleFont = nullptr;
    QSpinBox *m_fontSize = nullptr;
    QTextEdit *m_consolePreview = nullptr;
    bool m_loading = false;
};
