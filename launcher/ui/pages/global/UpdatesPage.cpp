#include "UpdatesPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

#include <algorithm>

#include "BuildConfig.h"

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
    m_variantBox = new QGroupBox(this);
    auto variantLayout = new QVBoxLayout(m_variantBox);
    m_variantLabel = new QLabel(m_variantBox);
    m_variantLabel->setWordWrap(true);
    variantLayout->addWidget(m_variantLabel);
    m_variant = new QComboBox(m_variantBox);
    variantLayout->addWidget(m_variant);
    layout->addWidget(m_variantBox);
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
        connect(updater.get(), &UpdaterV2::Checker::variantsKnown, this, &UpdatesPage::updateVariants);
    }
    retranslate();
    loadSettings();
}

void UpdatesPage::updateVariants()
{
    // what is picked now, or the setting before anything was
    auto picked = m_variant->count() ? m_variant->currentData().toString() : APPLICATION->settings()->get("UpdateVariant").toString();
    QList<UpdaterV2::Variant> runnable;
    QString recommended;
    auto updater = APPLICATION->updaterV2();
    if(updater)
    {
        updater->lastVariants(runnable, recommended);
    }
    auto own = BuildConfig.UPDATER_VARIANT;
    auto name = [&](const QString &id)
    {
        for(auto &v: runnable)
        {
            if(v.id == id)
            {
                return v.name;
            }
        }
        return id;
    };

    m_variant->clear();
    m_variant->addItem(recommended.isEmpty() ? tr("Recommended") : tr("Recommended: %1").arg(name(recommended)), QString());
    for(auto &v: runnable)
    {
        m_variant->addItem(v.id == own ? tr("%1 (this installation)").arg(v.name) : v.name, v.id);
    }
    // a choice the last check didn't list (no check yet, or it no longer runs here) stays as it is
    if(!picked.isEmpty() && m_variant->findData(picked) < 0)
    {
        m_variant->addItem(picked == own ? tr("%1 (this installation)").arg(picked) : picked, picked);
    }
    m_variant->setCurrentIndex(std::max(0, m_variant->findData(picked)));
    // only when there is something to choose
    m_variantBox->setVisible(runnable.size() > 1 || !picked.isEmpty());
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
    bool switched = false;
    auto tier = UpdaterV2::tierName(m_tierWidget->tier());
    if(tier != s->get("UpdateTier").toString())
    {
        s->set("UpdateTier", tier);
        switched = true;
    }
    auto variant = m_variant->currentData().toString();
    if(variant != s->get("UpdateVariant").toString())
    {
        s->set("UpdateVariant", variant);
        switched = true;
    }
    // a switch is the one time an older build is offered (as a downgrade, with a warning), and
    // another variant of the installed build is offered right away
    auto updater = APPLICATION->updaterV2();
    if(switched && updater && APPLICATION->updatesAreAllowed())
    {
        updater->check(m_tierWidget->tier(), true, true);
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
    m_variant->clear();
    updateVariants();
}

void UpdatesPage::retranslate()
{
    m_autoUpdate->setText(tr("Check for updates on start"));
    m_tierBox->setTitle(tr("Update tier"));
    m_variantBox->setTitle(tr("Version for this computer"));
    m_variantLabel->setText(tr("This computer can run more than one version of %1. The recommended one is the newest "
                               "that runs here; when there is a newer one, %1 switches to it unless you choose another here.")
                                .arg(BuildConfig.LAUNCHER_NAME));
    // the item names are translated too
    updateVariants();
}

void UpdatesPage::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}
