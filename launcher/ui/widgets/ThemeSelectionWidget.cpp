#include "ThemeSelectionWidget.h"

#include <QButtonGroup>
#include <QEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QRadioButton>
#include <QToolButton>
#include <QPushButton>
#include <QVBoxLayout>

#include "Application.h"
#include "Colors.h"
#include "DetailRowDelegate.h"
#include "ThemePreview.h"
#include "ui/themes/ThemeList.h"

namespace {
// ThemePreview draws for 132 x 84; the rows show it smaller
const QSize PREVIEW_SIZE(99, 63);
const qreal PREVIEW_SCALE = 0.75;
}

ThemeSelectionWidget::ThemeSelectionWidget(QWidget *parent) : QWidget(parent)
{
    m_manager = APPLICATION->themeManager();
    m_choice = m_manager->savedChoice();

    auto layout = new QVBoxLayout(this);

    // the style on top
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

    layout->addLayout(choices);

    m_themeNote = new QLabel(this);
    m_themeNote->setWordWrap(true);
    layout->addWidget(m_themeNote);

    // the themes: a small window in each one's colours, with the style chosen
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
    // a right click opens the row's menu without choosing the row (eventFilter)
    m_view->viewport()->installEventFilter(this);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_view, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        emit rowMenuRequested(m_view->indexAt(pos), m_view->viewport()->mapToGlobal(pos));
    });
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

    // the System theme and themes with the system's accent show the system's current look
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
    // the previews show the style (and the system's accent, with themes that follow it)
    m_view->viewport()->update();
    m_themeNote->setVisible(!themesAvailable);

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
    updateControls();
}

bool ThemeSelectionWidget::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == m_view->viewport() && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick)
       && static_cast<QMouseEvent *>(event)->button() == Qt::RightButton)
    {
        return true;
    }
    return QWidget::eventFilter(watched, event);
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
