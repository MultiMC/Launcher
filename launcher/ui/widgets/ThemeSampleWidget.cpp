#include "ThemeSampleWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStyleOptionFrame>
#include <QStylePainter>
#include <QTabBar>
#include <QVBoxLayout>

namespace {
// A tooltip as the style draws one (its panel, the tooltip colours), in place: a real one
// takes the application's tooltip palette, not the sample's, and only shows on hover
class ToolTipSample : public QLabel
{
public:
    ToolTipSample(const QString &text, QWidget *parent) : QLabel(text, parent)
    {
        setForegroundRole(QPalette::ToolTipText);
        setBackgroundRole(QPalette::ToolTipBase);
        // as QToolTip lays out its label
        setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, this));
        setFrameStyle(QFrame::NoFrame);
        setAlignment(Qt::AlignLeft);
        setIndent(1);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        // the tooltip's panel, then the text on it
        QStylePainter p(this);
        QStyleOptionFrame option;
        option.initFrom(this);
        p.drawPrimitive(QStyle::PE_PanelTipLabel, option);
        p.end();
        QLabel::paintEvent(event);
    }
};
}

ThemeSampleWidget::ThemeSampleWidget(QWidget *parent) : QWidget(parent)
{
    // on the window's background, as the launcher's windows are
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Window);
    auto page = this;
    auto pageLayout = new QVBoxLayout(this);

    // tabs, as in the settings and the instance windows
    auto tabs = new QTabBar(page);
    tabs->addTab(tr("Instances"));
    tabs->addTab(tr("Settings"));
    tabs->setDrawBase(false);
    tabs->setExpanding(false);
    pageLayout->addWidget(tabs);

    // a list with a selection and alternate rows: instances, versions, mods
    auto list = new QListWidget(page);
    list->setAlternatingRowColors(true);
    for (auto name : {tr("Survival world"), tr("Modded adventure"), tr("Creative builds"), tr("Snapshot testing")})
    {
        list->addItem(name);
    }
    list->setCurrentRow(1);
    list->setMaximumHeight(list->sizeHintForRow(0) * 4 + 2 * list->frameWidth() + 4);
    pageLayout->addWidget(list);

    // fields: one with selected text, a combo box, a spin box
    auto fields = new QGridLayout();
    auto name = new QLineEdit(tr("Modded adventure"), page);
    name->setSelection(0, 6);
    fields->addWidget(new QLabel(tr("Name:"), page), 0, 0);
    fields->addWidget(name, 0, 1);
    auto version = new QComboBox(page);
    version->addItems({"1.21.4", "1.20.1", "1.12.2"});
    fields->addWidget(new QLabel(tr("Version:"), page), 1, 0);
    fields->addWidget(version, 1, 1);
    auto memory = new QSpinBox(page);
    memory->setRange(512, 65536);
    memory->setValue(4096);
    memory->setSuffix(" MiB");
    fields->addWidget(new QLabel(tr("Memory:"), page), 2, 0);
    fields->addWidget(memory, 2, 1);
    pageLayout->addLayout(fields);

    // the boxes, on and off
    auto boxes = new QHBoxLayout();
    auto check = new QCheckBox(tr("Check box"), page);
    check->setChecked(true);
    auto radio = new QRadioButton(tr("Radio button"), page);
    radio->setChecked(true);
    boxes->addWidget(check);
    boxes->addWidget(radio);
    boxes->addStretch(1);
    pageLayout->addLayout(boxes);

    // progress, as while downloading
    auto progress = new QProgressBar(page);
    progress->setValue(60);
    pageLayout->addWidget(progress);

    // a link, as in the news and the about window; a tooltip, shown all the time
    auto linkRow = new QHBoxLayout();
    m_link = new QLabel(page);
    m_link->setTextInteractionFlags(Qt::NoTextInteraction);
    setLinkText(palette());
    linkRow->addWidget(m_link);
    linkRow->addStretch(1);
    linkRow->addWidget(new ToolTipSample(tr("A tooltip"), page));
    pageLayout->addLayout(linkRow);

    // buttons: the default one, another, a disabled one
    auto buttons = new QHBoxLayout();
    auto launch = new QPushButton(tr("Launch"), page);
    launch->setDefault(true);
    buttons->addWidget(launch);
    buttons->addWidget(new QPushButton(tr("Edit"), page));
    auto disabled = new QPushButton(tr("Disabled"), page);
    disabled->setEnabled(false);
    buttons->addWidget(disabled);
    buttons->addStretch(1);
    pageLayout->addLayout(buttons);

    // the disabled colours: a box of things that can't be used right now
    auto off = new QGroupBox(tr("Disabled"), page);
    auto offLayout = new QHBoxLayout(off);
    auto offEdit = new QLineEdit(tr("Some text"), off);
    auto offCheck = new QCheckBox(tr("Check box"), off);
    offCheck->setChecked(true);
    offLayout->addWidget(offEdit);
    offLayout->addWidget(offCheck);
    off->setEnabled(false);
    pageLayout->addWidget(off);
    pageLayout->addStretch(1);
}

void ThemeSampleWidget::setSamplePalette(const QPalette &palette)
{
    setPalette(palette);
    for (auto widget : findChildren<QWidget *>())
    {
        widget->setPalette(palette);
    }
    setLinkText(palette);
}

// A label's links take the application palette's Link colour when its text is read (Qt's
// HTML import), not the label's: the colour goes into the link itself
void ThemeSampleWidget::setLinkText(const QPalette &palette)
{
    m_link->setText(QString("<a href=\"#\" style=\"color: %1\">%2</a>").arg(palette.color(QPalette::Link).name(), tr("A link")));
}
