/* Copyright 2026 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "UpdatesWizardPage.h"

#include <QVBoxLayout>

#include <Application.h>
#include <BuildConfig.h>

#include "settings/SettingsObject.h"
#include "ui/widgets/UpdateTierWidget.h"

UpdatesWizardPage::UpdatesWizardPage(QWidget *parent) : BaseWizardPage(parent)
{
    setObjectName(QStringLiteral("updatesPage"));
    auto layout = new QVBoxLayout(this);
    m_tierWidget = new UpdateTierWidget(this);
    layout->addWidget(m_tierWidget);
    layout->addStretch(1);
    retranslate();
}

bool UpdatesWizardPage::validatePage()
{
    APPLICATION->settings()->set("UpdateTier", UpdaterV2::tierName(m_tierWidget->tier()));
    return true;
}

void UpdatesWizardPage::retranslate()
{
    setTitle(tr("Updates"));
    setSubTitle(tr("Choose how often %1 updates. You can change it later in the settings.").arg(BuildConfig.LAUNCHER_NAME));
}
