#include "ThemeWizardPage.h"

#include <QTabWidget>
#include <QVBoxLayout>

#include <Application.h>
#include <BuildConfig.h>

#include "ui/themes/ThemeManager.h"
#include "settings/SettingsObject.h"
#include "ui/widgets/IconThemeSelectionWidget.h"
#include "ui/widgets/ThemeSelectionWidget.h"

ThemeWizardPage::ThemeWizardPage(QWidget *parent) : BaseWizardPage(parent)
{
    setObjectName(QStringLiteral("themePage"));
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_tabs = new QTabWidget(this);
    layout->addWidget(m_tabs);
    m_themeWidget = new ThemeSelectionWidget(m_tabs);
    m_tabs->addTab(m_themeWidget, QString());
    connect(m_themeWidget, &ThemeSelectionWidget::choiceChanged, this, [](const ThemeManager::Choice &choice)
    {
        APPLICATION->themeManager()->apply(choice);
    });

    m_iconsWidget = new IconThemeSelectionWidget(m_tabs);
    m_iconsWidget->setIconTheme(APPLICATION->settings()->get("IconTheme").toString());
    m_tabs->addTab(m_iconsWidget, QString());
    // always set (to the default until chosen): saved at once, unlike the theme
    connect(m_iconsWidget, &IconThemeSelectionWidget::iconThemeChanged, this, [](const QString &id)
    {
        APPLICATION->settings()->set("IconTheme", id);
        APPLICATION->setIconTheme(id);
    });
    retranslate();
}

bool ThemeWizardPage::validatePage()
{
    APPLICATION->themeManager()->save(m_themeWidget->choice());
    return true;
}

void ThemeWizardPage::retranslate()
{
    setTitle(tr("Appearance"));
    m_tabs->setTabText(m_tabs->indexOf(m_themeWidget), tr("Theme"));
    m_tabs->setTabText(m_tabs->indexOf(m_iconsWidget), tr("Icons"));
    setSubTitle(tr("Choose how %1 looks. You can change it later in the settings.").arg(BuildConfig.LAUNCHER_NAME));
}
