#include "ThemeSelectionWidget.h"

#include <QButtonGroup>
#include <QEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPainter>
#include <QRadioButton>
#include <QToolButton>
#include <QVBoxLayout>

#include "Application.h"
#include "Colors.h"
#include "DetailRowDelegate.h"
#include "ThemePreview.h"
#include "ui/dialogs/AccentColorDialog.h"
#include "ui/themes/ThemeList.h"

namespace {
// A small square of a colour, for the accent choices: a rounded square, its edge a little darker
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

// ThemePreview draws for 132 x 84; the rows show it smaller
const QSize PREVIEW_SIZE(99, 63);
const qreal PREVIEW_SCALE = 0.75;
}

ThemeSelectionWidget::ThemeSelectionWidget(QWidget *parent) : QWidget(parent)
{
    m_manager = APPLICATION->themeManager();
    m_choice = m_manager->savedChoice();

    auto layout = new QVBoxLayout(this);

    // the style and the accent on top, one row each
    auto choices = new QFormLayout();
    m_styleLabel = new QLabel(this);
    m_styleRow = new QWidget(this);
    auto styleLayout = new QHBoxLayout(m_styleRow);
    styleLayout->setContentsMargins(0, 0, 0, 0);
    styleLayout->setSpacing(16);
    m_styleGroup = new QButtonGroup(this);
    m_fusion = new QRadioButton(m_styleRow);
    m_native = new QRadioButton(m_styleRow);
    m_styleGroup->addButton(m_fusion, int(ThemeManager::Style::Fusion));
    m_styleGroup->addButton(m_native, int(ThemeManager::Style::Native));
    styleLayout->addWidget(m_fusion);
    styleLayout->addWidget(m_native);
    styleLayout->addStretch(1);
    choices->addRow(m_styleLabel, m_styleRow);
    connect(m_styleGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, [this](int id) {
        m_choice.style = ThemeManager::Style(id);
        userChanged();
    });
    m_styleLabel->setVisible(m_manager->hasNativeStyleChoice());
    m_styleRow->setVisible(m_manager->hasNativeStyleChoice());

    m_accentLabel = new QLabel(this);
    m_accentRow = new QWidget(this);
    // each swatch right next to its own choice, the choices well apart
    auto accentLayout = new QHBoxLayout(m_accentRow);
    accentLayout->setContentsMargins(0, 0, 0, 0);
    accentLayout->setSpacing(16);
    m_accentGroup = new QButtonGroup(this);
    m_themeAccent = new QRadioButton(m_accentRow);
    m_systemAccent = new QRadioButton(m_accentRow);
    m_customAccent = new QRadioButton(m_accentRow);
    m_customColor = new QToolButton(m_accentRow);
    m_accentGroup->addButton(m_themeAccent, int(ThemeManager::AccentSource::Theme));
    m_accentGroup->addButton(m_systemAccent, int(ThemeManager::AccentSource::System));
    m_accentGroup->addButton(m_customAccent, int(ThemeManager::AccentSource::Custom));
    auto custom = new QHBoxLayout();
    custom->setSpacing(4);
    custom->addWidget(m_customAccent);
    custom->addWidget(m_customColor);
    accentLayout->addWidget(m_themeAccent);
    accentLayout->addWidget(m_systemAccent);
    accentLayout->addLayout(custom);
    accentLayout->addStretch(1);
    choices->addRow(m_accentLabel, m_accentRow);
    connect(m_accentGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, [this](int id) {
        m_choice.accentSource = ThemeManager::AccentSource(id);
        userChanged();
    });
    connect(m_customColor, &QToolButton::clicked, this, &ThemeSelectionWidget::pickCustomAccent);
    layout->addLayout(choices);

    m_themeNote = new QLabel(this);
    m_themeNote->setWordWrap(true);
    layout->addWidget(m_themeNote);

    // the themes: a small window in each one's colours, with the accent and style chosen
    m_list = m_manager->themeList();
    m_view = new QListView(this);
    m_view->setUniformItemSizes(true);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto paintPreview = [this](QPainter &p, const QRectF &r, const QModelIndex &index) {
        p.save();
        p.translate(r.topLeft());
        p.scale(PREVIEW_SCALE, PREVIEW_SCALE);
        ThemePreview::paint(p, QRectF(QPointF(0, 0), QSizeF(PREVIEW_SIZE) / PREVIEW_SCALE),
                            m_manager->previewPalette(index.data(ThemeList::IdRole).toString(), m_choice));
        p.restore();
    };
    m_view->setItemDelegate(new DetailRowDelegate(PREVIEW_SIZE, ThemeList::DetailsRole, paintPreview, m_view));
    m_view->setModel(m_list);
    layout->addWidget(m_view, 1);
    connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current) {
        if(m_updating || !current.isValid())
            return;
        const QString id = current.data(ThemeList::IdRole).toString();
        // with a style that can't take our themes, only the platform's look applies
        if(!themeSelectable(id))
        {
            updateControls();
            return;
        }
        selectTheme(id);
    });
    // themes that come and go keep the selection where it was
    connect(m_list, &QAbstractItemModel::modelReset, this, &ThemeSelectionWidget::updateControls);

    // the System theme and the system's accent show the system's current look
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

bool ThemeSelectionWidget::themeSelectable(const QString &id) const
{
    const auto style = m_manager->hasNativeStyleChoice() ? m_choice.style : ThemeManager::Style::Fusion;
    return m_manager->themeChoiceAvailable(style) || id == "system";
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
    m_updating = true;
    const QModelIndex current = m_list->indexOf(m_choice.theme);
    m_view->selectionModel()->setCurrentIndex(current, QItemSelectionModel::ClearAndSelect);
    m_view->scrollTo(current);
    m_updating = false;
    // the previews show the accent and the style
    m_view->viewport()->update();
    m_themeNote->setVisible(!themesAvailable);

    const bool accentsAvailable = m_manager->accentChoiceAvailable(style);
    const auto systemAccent = m_manager->systemAccent();
    m_accentRow->setEnabled(accentsAvailable);
    m_accentRow->setToolTip(accentsAvailable ? QString() : tr("The %1 style uses the system's own accent.").arg(m_manager->nativeStyleName()));
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
}

void ThemeSelectionWidget::retranslate()
{
    m_themeNote->setText(tr("The %1 style only shows the system's own colors.").arg(m_manager->nativeStyleName()));
    m_styleLabel->setText(tr("Style:"));
    m_fusion->setText(tr("Same everywhere (Fusion)"));
    m_fusion->setToolTip(tr("Looks the same on every system, with every theme and accent."));
    m_native->setText(tr("Native (%1)").arg(m_manager->nativeStyleName()));
    m_native->setToolTip(tr("Looks like other applications on this system. Layouts may fit less well, and what the themes and accents can change depends on the style."));
    m_accentLabel->setText(tr("Accent color:"));
    m_themeAccent->setText(tr("Theme's own"));
    m_systemAccent->setText(tr("System"));
    m_customAccent->setText(tr("Custom"));
    updateControls();
}

void ThemeSelectionWidget::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        retranslate();
        // the built-in themes' names
        m_view->viewport()->update();
    }
    QWidget::changeEvent(event);
}
