#include "ThemeSelectionWidget.h"

#include <QButtonGroup>
#include <QEvent>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QPainter>
#include <QRadioButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <functional>

#include "Application.h"
#include "Colors.h"
#include "ui/dialogs/AccentColorDialog.h"
#include "ui/themes/ITheme.h"
#include "SelectionCard.h"
#include "ThemePreview.h"

namespace {
// A small square of a colour, for the accent choices
QIcon swatch(const QColor &color)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color.darker(150), 1));
    p.setBrush(color);
    p.drawRoundedRect(QRectF(0.5, 0.5, 15, 15), 3, 3);
    return QIcon(pixmap);
}
}

/**
 * One theme: a small window in its colours (title bar, a list with a selected row, a field,
 * a button and a link)
 */
class ThemeCard : public SelectionCard
{
public:
    ThemeCard(const QString &id, std::function<void()> clicked, QWidget *parent)
        : SelectionCard(id, QSize(132, 84), clicked, parent)
    {
    }

    void setPreview(const QString &name, const QPalette &preview)
    {
        m_preview = preview;
        setName(name);
    }

protected:
    void paintPicture(QPainter &p, const QRectF &r) override
    {
        ThemePreview::paint(p, r, m_preview);
    }

private: /* data */
    QPalette m_preview;
};

ThemeSelectionWidget::ThemeSelectionWidget(QWidget *parent) : QWidget(parent)
{
    m_manager = APPLICATION->themeManager();
    m_choice = m_manager->savedChoice();

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto cards = new QHBoxLayout();
    cards->addStretch(1);
    for(auto theme: m_manager->themes())
    {
        auto id = theme->id();
        auto card = new ThemeCard(id, [this, id]() { selectTheme(id); }, this);
        m_cards.append(card);
        cards->addWidget(card);
    }
    cards->addStretch(1);
    layout->addLayout(cards);
    m_themeNote = new QLabel(this);
    m_themeNote->setWordWrap(true);
    m_themeNote->setAlignment(Qt::AlignHCenter);
    layout->addWidget(m_themeNote);

    m_accentBox = new QGroupBox(this);
    // centred, the choices well apart, each swatch right next to its own choice
    auto accentLayout = new QHBoxLayout(m_accentBox);
    accentLayout->setSpacing(24);
    m_accentGroup = new QButtonGroup(this);
    m_themeAccent = new QRadioButton(m_accentBox);
    m_systemAccent = new QRadioButton(m_accentBox);
    m_customAccent = new QRadioButton(m_accentBox);
    m_customColor = new QToolButton(m_accentBox);
    m_accentGroup->addButton(m_themeAccent, int(ThemeManager::AccentSource::Theme));
    m_accentGroup->addButton(m_systemAccent, int(ThemeManager::AccentSource::System));
    m_accentGroup->addButton(m_customAccent, int(ThemeManager::AccentSource::Custom));
    auto custom = new QHBoxLayout();
    custom->setSpacing(4);
    custom->addWidget(m_customAccent);
    custom->addWidget(m_customColor);
    accentLayout->addStretch(1);
    accentLayout->addWidget(m_themeAccent);
    accentLayout->addWidget(m_systemAccent);
    accentLayout->addLayout(custom);
    accentLayout->addStretch(1);
    layout->addWidget(m_accentBox);
    connect(m_accentGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, [this](int id) {
        m_choice.accentSource = ThemeManager::AccentSource(id);
        userChanged();
    });
    connect(m_customColor, &QToolButton::clicked, this, &ThemeSelectionWidget::pickCustomAccent);

    m_styleBox = new QGroupBox(this);
    auto styleLayout = new QVBoxLayout(m_styleBox);
    auto styleButtons = new QHBoxLayout();
    styleButtons->setSpacing(24);
    m_styleGroup = new QButtonGroup(this);
    m_fusion = new QRadioButton(m_styleBox);
    m_native = new QRadioButton(m_styleBox);
    m_styleGroup->addButton(m_fusion, int(ThemeManager::Style::Fusion));
    m_styleGroup->addButton(m_native, int(ThemeManager::Style::Native));
    styleButtons->addStretch(1);
    styleButtons->addWidget(m_fusion);
    styleButtons->addWidget(m_native);
    styleButtons->addStretch(1);
    styleLayout->addLayout(styleButtons);
    m_styleDescription = new QLabel(m_styleBox);
    m_styleDescription->setWordWrap(true);
    m_styleDescription->setAlignment(Qt::AlignHCenter);
    // windows don't get taller for wrapped text: two lines, whichever description it is
    m_styleDescription->setMinimumHeight(2 * m_styleDescription->fontMetrics().lineSpacing());
    styleLayout->addWidget(m_styleDescription);
    layout->addWidget(m_styleBox);
    m_styleBox->setVisible(m_manager->hasNativeStyleChoice());
    connect(m_styleGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, [this](int id) {
        m_choice.style = ThemeManager::Style(id);
        userChanged();
    });

    // the System card and the system's accent show the system's current look
    connect(m_manager, &ThemeManager::systemChanged, this, &ThemeSelectionWidget::updateControls);

    retranslate();
    updateControls();
}

ThemeManager::Choice ThemeSelectionWidget::choice() const
{
    return m_choice;
}

void ThemeSelectionWidget::setChoice(const ThemeManager::Choice &choice)
{
    m_choice = choice;
    updateControls();
}

void ThemeSelectionWidget::selectTheme(const QString &id)
{
    m_choice.theme = id;
    userChanged();
}

void ThemeSelectionWidget::pickCustomAccent()
{
    // the chosen theme with the colour as its custom accent
    auto preview = [this](const QColor &color) {
        ThemeManager::Choice choice = m_choice;
        choice.accentSource = ThemeManager::AccentSource::Custom;
        choice.accentColor = color;
        return m_manager->previewPalette(choice.theme, choice);
    };
    AccentColorDialog dialog(m_choice.accentColor, m_manager->systemAccent().color, preview, this);
    if(dialog.exec() != QDialog::Accepted)
    {
        return;
    }
    m_choice.accentColor = dialog.color();
    m_choice.accentSource = ThemeManager::AccentSource::Custom;
    userChanged();
}

void ThemeSelectionWidget::userChanged()
{
    updateControls();
    emit choiceChanged(m_choice);
}

void ThemeSelectionWidget::updateControls()
{
    const auto style = m_manager->hasNativeStyleChoice() ? m_choice.style : ThemeManager::Style::Fusion;
    const bool themesAvailable = m_manager->themeChoiceAvailable(style);
    QMap<QString, QString> names;
    for(auto theme: m_manager->themes())
    {
        names[theme->id()] = theme->name();
    }
    for(auto card: m_cards)
    {
        card->setPreview(names.value(card->id()), m_manager->previewPalette(card->id(), m_choice));
        card->setSelected(card->id() == m_choice.theme);
        // with a style that can't take our themes, only the platform's look applies
        card->setEnabled(themesAvailable || card->id() == "system");
    }
    m_themeNote->setVisible(!themesAvailable);

    const bool accentsAvailable = m_manager->accentChoiceAvailable(style);
    const auto systemAccent = m_manager->systemAccent();
    m_accentBox->setEnabled(accentsAvailable);
    m_accentBox->setToolTip(accentsAvailable ? QString() : tr("The %1 style uses the system's own accent.").arg(m_manager->nativeStyleName()));
    m_systemAccent->setEnabled(systemAccent.isValid());
    m_systemAccent->setIcon(systemAccent.isValid() ? swatch(systemAccent.color) : QIcon());
    const bool customAvailable = m_manager->customAccentAvailable(style);
    m_customAccent->setEnabled(customAvailable);
    m_customColor->setEnabled(customAvailable);
    const QString customTip = customAvailable ? QString() : tr("The %1 style draws its controls in the system's accent; a custom accent applies with Fusion.").arg(m_manager->nativeStyleName());
    m_customAccent->setToolTip(customTip);
    m_customColor->setToolTip(customAvailable ? tr("Choose the accent color") : customTip);
    m_customColor->setIcon(swatch(m_choice.accentColor));
    auto accentButton = m_accentGroup->button(int(m_choice.accentSource));
    if(accentButton)
    {
        accentButton->setChecked(true);
    }

    auto styleButton = m_styleGroup->button(int(style));
    if(styleButton)
    {
        styleButton->setChecked(true);
    }
    if(style == ThemeManager::Style::Native)
    {
        m_styleDescription->setText(tr("Looks like other applications on this system. Layouts may fit less well, and what the themes and accents can change depends on the style."));
    }
    else
    {
        m_styleDescription->setText(tr("Looks the same on every system, with every theme and accent."));
    }
}

void ThemeSelectionWidget::retranslate()
{
    m_themeNote->setText(tr("The %1 style only shows the system's own colors.").arg(m_manager->nativeStyleName()));
    m_accentBox->setTitle(tr("Accent color"));
    m_themeAccent->setText(tr("Theme's own"));
    m_systemAccent->setText(tr("System"));
    m_customAccent->setText(tr("Custom"));
    m_styleBox->setTitle(tr("Style"));
    m_fusion->setText(tr("Same everywhere (Fusion)"));
    m_native->setText(tr("Native (%1)").arg(m_manager->nativeStyleName()));
    updateControls();
}

void ThemeSelectionWidget::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}
