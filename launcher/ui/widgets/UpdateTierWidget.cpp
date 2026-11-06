#include "UpdateTierWidget.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

#include <rainbow.h>

#include "BuildConfig.h"
#include "TierLever.h"

/**
 * One tier next to the lever: its name, the build it is on, and a line about it. The selected
 * one is shaded in the highlight color; clicking one selects it.
 */
class TierRow : public QWidget
{
public:
    TierRow(std::function<void()> clicked, QWidget *parent) : QWidget(parent), m_clicked(clicked)
    {
        setCursor(Qt::PointingHandCursor);
        auto layout = new QGridLayout(this);
        layout->setContentsMargins(8, 4, 8, 4);
        layout->setVerticalSpacing(2);
        title = new QLabel(this);
        auto font = title->font();
        font.setBold(true);
        title->setFont(font);
        build = new QLabel(this);
        build->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        updateBuildColor();
        summary = new QLabel(this);
        summary->setWordWrap(true);
        layout->addWidget(title, 0, 0);
        layout->addWidget(build, 0, 1);
        layout->addWidget(summary, 1, 0, 1, 2);
        layout->setColumnStretch(0, 1);
    }

    void setSelected(bool selected)
    {
        m_selected = selected;
        update();
    }

    QLabel *title = nullptr;
    QLabel *build = nullptr;
    QLabel *summary = nullptr;

protected:
    void changeEvent(QEvent *event) override
    {
        if(event->type() == QEvent::PaletteChange)
        {
            updateBuildColor();
        }
        QWidget::changeEvent(event);
    }

    void paintEvent(QPaintEvent *) override
    {
        if(!m_selected)
        {
            return;
        }
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        auto highlight = palette().color(QPalette::Highlight);
        auto fill = highlight;
        fill.setAlpha(48);
        p.setPen(QPen(highlight, 1));
        p.setBrush(fill);
        p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if(event->button() == Qt::LeftButton)
        {
            m_clicked();
        }
    }

private:
    // dimmed text: the text color moved part of the way toward the background, readable on
    // light and dark schemes alike (unlike the Mid role, which is frame shading)
    void updateBuildColor()
    {
        auto dimmed = Rainbow::mix(palette().color(QPalette::WindowText), palette().color(QPalette::Window), 0.4);
        auto buildPalette = build->palette();
        buildPalette.setColor(QPalette::WindowText, dimmed);
        build->setPalette(buildPalette);
    }

private: /* data */
    std::function<void()> m_clicked;
    bool m_selected = false;
};

/**
 * Never lays its items out in less height than they need. When the description grows, Qt first
 * lays out this widget at its old height, and only then do the parents make it taller. A plain
 * layout would take the difference from the lever in between.
 */
class FullHeightLayout : public QVBoxLayout
{
public:
    using QVBoxLayout::QVBoxLayout;

    void setGeometry(const QRect &rect) override
    {
        QRect full = rect;
        full.setHeight(std::max(rect.height(), heightForWidth(rect.width())));
        QVBoxLayout::setGeometry(full);
    }
};

UpdateTierWidget::UpdateTierWidget(QWidget *parent) : QWidget(parent)
{
    auto layout = new FullHeightLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto leverLayout = new QHBoxLayout();
    m_lever = new TierLever(3, this);
    leverLayout->addWidget(m_lever);
    auto rowsLayout = new QVBoxLayout();
    rowsLayout->setSpacing(0);
    // top to bottom: the tiers in their order, Latest first
    for(int position = 0; position < 3; position++)
    {
        auto row = new TierRow([this, position]() { select(position, true); emit tierChanged(tier()); }, this);
        rowsLayout->addWidget(row, 1);
        m_rows.append(row);
    }
    leverLayout->addLayout(rowsLayout, 1);
    layout->addLayout(leverLayout);
    connect(m_lever, &TierLever::positionChanged, this, [this](int position)
    {
        select(position, true);
        emit tierChanged(tier());
    });

    auto frame = new QFrame(this);
    frame->setFrameShape(QFrame::StyledPanel);
    auto frameLayout = new QVBoxLayout(frame);
    m_description = new QLabel(frame);
    m_description->setWordWrap(true);
    m_description->setTextFormat(Qt::RichText);
    frameLayout->addWidget(m_description);
    // any spare height goes here, not to the lever
    layout->addWidget(frame, 1);

    retranslate();
    select(int(UpdaterV2::Tier::Stable), false);
}

UpdaterV2::Tier UpdateTierWidget::tier() const
{
    return UpdaterV2::Tier(m_lever->position());
}

void UpdateTierWidget::setTier(UpdaterV2::Tier tier)
{
    select(int(tier), false);
}

void UpdateTierWidget::select(int position, bool animated)
{
    if(m_lever->position() != position || !animated)
    {
        m_lever->setPosition(position, animated);
    }
    for(int i = 0; i < m_rows.size(); i++)
    {
        m_rows[i]->setSelected(i == position);
    }
    updateDescription();
}

void UpdateTierWidget::setTags(const UpdaterV2::TagsDocument &tags)
{
    m_tags = tags;
    m_haveTags = true;
    updateBuilds();
}

void UpdateTierWidget::updateBuilds()
{
    qint64 builds[] = {m_tags.latest, m_tags.testing, m_tags.stable};
    for(int i = 0; i < m_rows.size(); i++)
    {
        auto label = m_rows[i]->build;
        if(!m_haveTags)
        {
            label->clear();
        }
        else if(builds[i] == BuildConfig.VERSION_BUILD)
        {
            label->setText(tr("build %1 (installed)").arg(builds[i]));
        }
        else
        {
            label->setText(tr("build %1").arg(builds[i]));
        }
    }
}

void UpdateTierWidget::retranslate()
{
    m_lever->setAccessibleName(tr("Update tier"));
    m_rows[0]->title->setText(tr("Latest"));
    m_rows[0]->summary->setText(tr("Every build, as soon as it is made."));
    m_rows[1]->title->setText(tr("Testing"));
    m_rows[1]->summary->setText(tr("Builds being tested to become Stable."));
    m_rows[2]->title->setText(tr("Stable"));
    m_rows[2]->summary->setText(tr("Builds that were tested for a long time."));
    updateBuilds();
    updateDescription();
}

void UpdateTierWidget::updateDescription()
{
    switch(tier())
    {
        case UpdaterV2::Tier::Latest:
            m_description->setText(tr(
                "<b>Latest</b> gets every change first, including ones that haven't been tested much yet. "
                "Updates come often, sometimes several in a day.<br/><br/>"
                "Pick this if you want the newest things and don't mind running into a problem now and then."));
            break;
        case UpdaterV2::Tier::Testing:
            m_description->setText(tr(
                "<b>Testing</b> gets the builds that are picked to become the next Stable, while they are tested. "
                "Updates come every now and then.<br/><br/>"
                "Problems are rare, and what happens on Testing decides what reaches Stable."));
            break;
        case UpdaterV2::Tier::Stable:
            m_description->setText(tr(
                "<b>Stable</b> gets builds only after they were on Testing long enough to be trusted. "
                "Updates come rarely.<br/><br/>"
                "Recommended for most people."));
            break;
    }
}

void UpdateTierWidget::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}
