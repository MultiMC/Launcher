#pragma once

#include <QWidget>

#include "ui/pages/BasePage.h"
#include <Application.h>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class UpdateTierWidget;

/**
 * The updater's settings: checking for updates on start, the tier (UpdateTierWidget), and which
 * variant to run when this computer can run several.
 * Only in builds that can update themselves (BuildConfig.UPDATER_ENABLED).

 */
class UpdatesPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit UpdatesPage(QWidget *parent = 0);
    virtual ~UpdatesPage() {};

    QString displayName() const override
    {
        return tr("Updates");
    }
    QIcon icon() const override
    {
        return APPLICATION->getThemedIcon("checkupdate");
    }
    QString id() const override
    {
        return "update-settings";
    }
    QString helpPage() const override
    {
        return "Launcher-settings";
    }
    bool apply() override;

    void changeEvent(QEvent *) override;

private:
    void applySettings();
    void loadSettings();
    void retranslate();
    // the variants of the tier's build that run here, from the last check
    void updateVariants();

private: /* data */
    QCheckBox *m_autoUpdate = nullptr;
    QGroupBox *m_tierBox = nullptr;
    UpdateTierWidget *m_tierWidget = nullptr;
    QGroupBox *m_variantBox = nullptr;
    QLabel *m_variantLabel = nullptr;
    QComboBox *m_variant = nullptr;
};
