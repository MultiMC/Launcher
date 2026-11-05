#pragma once

#include <QWidget>

#include "ui/pages/BasePage.h"
#include <Application.h>

class QCheckBox;
class QGroupBox;
class UpdateTierWidget;

/**
 * Updater v2's settings: checking for updates on start, and the tier (UpdateTierWidget).
 * Only in builds with updater v2; builds with updater v1 keep their update settings on the
 * Launcher page.
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

private: /* data */
    QCheckBox *m_autoUpdate = nullptr;
    QGroupBox *m_tierBox = nullptr;
    UpdateTierWidget *m_tierWidget = nullptr;
};
