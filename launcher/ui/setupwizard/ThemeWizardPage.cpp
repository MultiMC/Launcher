#include "ThemeWizardPage.h"

#include <QGroupBox>
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
    // the previews don't shrink: the wizard grows to fit them
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    m_themeBox = new QGroupBox(this);
    auto themeLayout = new QVBoxLayout(m_themeBox);
    m_themeWidget = new ThemeSelectionWidget(m_themeBox);
    themeLayout->addWidget(m_themeWidget);
    layout->addWidget(m_themeBox);
    connect(m_themeWidget, &ThemeSelectionWidget::choiceChanged, this, [](const ThemeManager::Choice &choice)
    {
        APPLICATION->themeManager()->apply(choice);
    });

    m_iconsBox = new QGroupBox(this);
    auto iconsLayout = new QVBoxLayout(m_iconsBox);
    m_iconsWidget = new IconThemeSelectionWidget(m_iconsBox);
    m_iconsWidget->setIconTheme(APPLICATION->settings()->get("IconTheme").toString());
    iconsLayout->addWidget(m_iconsWidget);
    layout->addWidget(m_iconsBox);
    // always set (to the default until chosen): saved at once, unlike the theme
    connect(m_iconsWidget, &IconThemeSelectionWidget::iconThemeChanged, this, [](const QString &id)
    {
        APPLICATION->settings()->set("IconTheme", id);
        APPLICATION->setIconTheme(id);
    });
    layout->addStretch(1);
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
    m_themeBox->setTitle(tr("Theme"));
    m_iconsBox->setTitle(tr("Icons"));
    setSubTitle(tr("Choose how %1 looks. You can change it later in the settings.").arg(BuildConfig.LAUNCHER_NAME));
}
