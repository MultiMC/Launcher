#include "AppearancePage.h"

#include <QGroupBox>
#include <QVBoxLayout>

#include "settings/SettingsObject.h"
#include "ui/widgets/IconThemeSelectionWidget.h"
#include "ui/widgets/ThemeSelectionWidget.h"

AppearancePage::AppearancePage(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("appearancePage"));
    auto layout = new QVBoxLayout(this);
    m_themeBox = new QGroupBox(this);
    auto themeLayout = new QVBoxLayout(m_themeBox);
    m_themeWidget = new ThemeSelectionWidget(m_themeBox);
    themeLayout->addWidget(m_themeWidget);
    // at once, like in the setup wizard: the dialog has no Cancel to take it back
    connect(m_themeWidget, &ThemeSelectionWidget::choiceChanged, this, [](const ThemeManager::Choice &choice)
    {
        auto manager = APPLICATION->themeManager();
        manager->save(choice);
        manager->apply(choice);
    });
    layout->addWidget(m_themeBox);

    m_iconsBox = new QGroupBox(this);
    auto iconsLayout = new QVBoxLayout(m_iconsBox);
    m_iconsWidget = new IconThemeSelectionWidget(m_iconsBox);
    iconsLayout->addWidget(m_iconsWidget);
    connect(m_iconsWidget, &IconThemeSelectionWidget::iconThemeChanged, this, [](const QString &id)
    {
        APPLICATION->settings()->set("IconTheme", id);
        APPLICATION->setIconTheme(id);
    });
    layout->addWidget(m_iconsBox);
    layout->addStretch(1);

    retranslate();
    loadSettings();
}

void AppearancePage::loadSettings()
{
    m_iconsWidget->setIconTheme(APPLICATION->settings()->get("IconTheme").toString());
    m_themeWidget->setChoice(APPLICATION->themeManager()->savedChoice());
}

void AppearancePage::retranslate()
{
    m_themeBox->setTitle(tr("Theme"));
    m_iconsBox->setTitle(tr("Icons"));
}

void AppearancePage::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}
