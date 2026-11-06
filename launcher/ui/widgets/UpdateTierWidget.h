#pragma once

#include <QWidget>

#include "updater/v2/Formats.h"
#include "updater/v2/Plan.h"

class QLabel;
class TierLever;
class TierRow;

/**
 * Choosing updater v2's tier (Design/Updater.md in the MultiMC repository): a lever with
 * Latest at the top and Stable at the bottom, a row for each tier next to it (clicking a row
 * moves the lever there), and the selected tier explained below. Shared by the Updates
 * settings page and the setup wizard. With the tags of a check, each row also says which
 * build the tier is on.
 */
class UpdateTierWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UpdateTierWidget(QWidget *parent = nullptr);
    virtual ~UpdateTierWidget() {};

    UpdaterV2::Tier tier() const;
    // without emitting tierChanged
    void setTier(UpdaterV2::Tier tier);
    void setTags(const UpdaterV2::TagsDocument &tags);

signals:
    void tierChanged(UpdaterV2::Tier tier);

protected:
    void changeEvent(QEvent *event) override;

private:
    void select(int position, bool animated);
    void retranslate();
    void updateBuilds();
    void updateDescription();

private: /* data */
    TierLever *m_lever = nullptr;
    QList<TierRow *> m_rows;
    QLabel *m_description = nullptr;
    UpdaterV2::TagsDocument m_tags;
    bool m_haveTags = false;
};
