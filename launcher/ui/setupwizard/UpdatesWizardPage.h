#pragma once

#include "BaseWizardPage.h"

class UpdateTierWidget;

/**
 * Choosing updater v2's tier, for anyone who hasn't yet (the UpdateTier setting is empty).
 */
class UpdatesWizardPage : public BaseWizardPage
{
    Q_OBJECT
public:
    explicit UpdatesWizardPage(QWidget *parent = Q_NULLPTR);
    virtual ~UpdatesWizardPage() {};

    bool validatePage() override;

protected:
    void retranslate() override;

private: /* data */
    UpdateTierWidget *m_tierWidget = nullptr;
};
