#include "UpdatesPage.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>

#include "settings/SettingsObject.h"
#include "ui/widgets/UpdateTierWidget.h"
#include "updater/v2/Checker.h"

UpdatesPage::UpdatesPage(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("updatesPage"));
    auto layout = new QVBoxLayout(this);
    m_autoUpdate = new QCheckBox(this);
    layout->addWidget(m_autoUpdate);
    m_tierBox = new QGroupBox(this);
    auto tierLayout = new QVBoxLayout(m_tierBox);
    m_tierWidget = new UpdateTierWidget(m_tierBox);
    tierLayout->addWidget(m_tierWidget);
    layout->addWidget(m_tierBox);
    layout->addStretch(1);

    // which build each tier is on, from the last check (none before the first)
    auto updater = APPLICATION->updaterV2();
    if(updater)
    {
        UpdaterV2::TagsDocument tags;
        if(updater->lastTags(tags))
        {
            m_tierWidget->setTags(tags);
        }
        connect(updater.get(), &UpdaterV2::Checker::tagsVerified, this, [this, updater]()
        {
            UpdaterV2::TagsDocument tags;
            if(updater->lastTags(tags))
            {
                m_tierWidget->setTags(tags);
            }
        });
    }
    retranslate();
    loadSettings();
}

bool UpdatesPage::apply()
{
    applySettings();
    return true;
}

void UpdatesPage::applySettings()
{
    auto s = APPLICATION->settings();
    s->set("AutoUpdate", m_autoUpdate->isChecked());
    auto tier = UpdaterV2::tierName(m_tierWidget->tier());
    if(tier != s->get("UpdateTier").toString())
    {
        s->set("UpdateTier", tier);
        // a switch is the one time an older build is offered (as a downgrade, with a warning)
        auto updater = APPLICATION->updaterV2();
        if(updater && APPLICATION->updatesAreAllowed())
        {
            updater->check(m_tierWidget->tier(), true, true);
        }
    }
}

void UpdatesPage::loadSettings()
{
    auto s = APPLICATION->settings();
    m_autoUpdate->setChecked(s->get("AutoUpdate").toBool());
    // not chosen yet (empty) is Stable
    UpdaterV2::Tier tier = UpdaterV2::Tier::Stable;
    UpdaterV2::parseTier(s->get("UpdateTier").toString(), tier);
    m_tierWidget->setTier(tier);
}

void UpdatesPage::retranslate()
{
    m_autoUpdate->setText(tr("Check for updates on start"));
    m_tierBox->setTitle(tr("Update tier"));
}

void UpdatesPage::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}
