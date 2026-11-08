#include "IconThemeSelectionWidget.h"

#include <QEvent>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

#include "Application.h"
#include "DetailRowDelegate.h"
#include "ui/themes/IconThemeList.h"
#include "ui/themes/ThemeManager.h"

namespace {
const int ICON_SIZE = 24;
const int ICON_GAP = 6;
}

IconThemeSelectionWidget::IconThemeSelectionWidget(QWidget *parent) : QWidget(parent)
{
    m_list = APPLICATION->themeManager()->iconThemeList();
    auto layout = new QVBoxLayout(this);
    m_view = new QListView(this);
    m_view->setUniformItemSizes(true);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // a few of the set's icons in a row
    auto paintIcons = [this](QPainter &p, const QRectF &r, const QModelIndex &index) {
        const auto icons = m_list->samples(index);
        for(int i = 0; i < icons.size(); i++)
        {
            QPointF at(r.left() + i * (ICON_SIZE + ICON_GAP), r.top());
            p.drawPixmap(QRectF(at, QSizeF(ICON_SIZE, ICON_SIZE)), icons[i], QRectF(icons[i].rect()));
        }
    };
    const QSize pictureSize(6 * ICON_SIZE + 5 * ICON_GAP, ICON_SIZE);
    m_view->setItemDelegate(new DetailRowDelegate(pictureSize, IconThemeList::DetailsRole, paintIcons, m_view));
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
        m_iconTheme = current.data(IconThemeList::IdRole).toString();
        emit iconThemeChanged(m_iconTheme);
    });
    // sets that come and go keep the selection where it was
    connect(m_list, &QAbstractItemModel::modelReset, this, &IconThemeSelectionWidget::showSelection);
}

QString IconThemeSelectionWidget::iconTheme() const
{
    return m_iconTheme;
}

void IconThemeSelectionWidget::setIconTheme(const QString &id)
{
    m_iconTheme = id;
    showSelection();
}

void IconThemeSelectionWidget::showSelection()
{
    m_updating = true;
    const QModelIndex index = m_list->indexOf(m_iconTheme);
    m_view->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
    m_view->scrollTo(index);
    m_updating = false;
}

bool IconThemeSelectionWidget::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == m_view->viewport() && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick)
       && static_cast<QMouseEvent *>(event)->button() == Qt::RightButton)
    {
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void IconThemeSelectionWidget::changeEvent(QEvent *event)
{
    // the built-in sets' names
    if(event->type() == QEvent::LanguageChange)
    {
        m_view->viewport()->update();
    }
    QWidget::changeEvent(event);
}
