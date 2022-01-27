#include "VersionSelectWidget.h"

#include <QProgressBar>
#include <QVBoxLayout>
#include <QHeaderView>

#include "VersionListView.h"
#include "VersionProxyModel.h"

#include "ui/dialogs/CustomMessageBox.h"

VersionSelectWidget::VersionSelectWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("VersionSelectWidget"));
    verticalLayout = new QVBoxLayout(this);
    verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);

    m_proxyModel = new VersionProxyModel(this);

    listView = new VersionListView(this);
    listView->setObjectName(QStringLiteral("listView"));
    listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listView->setAlternatingRowColors(true);
    listView->setRootIsDecorated(false);
    listView->setItemsExpandable(false);
    listView->setWordWrap(true);
    listView->header()->setCascadingSectionResizes(true);
    listView->header()->setStretchLastSection(false);
    listView->setModel(m_proxyModel);
    verticalLayout->addWidget(listView);

    sneakyProgressBar = new QProgressBar(this);
    sneakyProgressBar->setObjectName(QStringLiteral("sneakyProgressBar"));
    sneakyProgressBar->setFormat(QStringLiteral("%p%"));
    verticalLayout->addWidget(sneakyProgressBar);
    sneakyProgressBar->setHidden(true);
    connect(listView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &VersionSelectWidget::currentRowChanged);

    QMetaObject::connectSlotsByName(this);
}

void VersionSelectWidget::setCurrentVersion(const QString& version)
{
    m_currentVersion = version;
    m_proxyModel->setCurrentVersion(version);
}

void VersionSelectWidget::setEmptyString(QString emptyString)
{
    listView->setEmptyString(emptyString);
}

void VersionSelectWidget::setEmptyErrorString(QString emptyErrorString)
{
    listView->setEmptyErrorString(emptyErrorString);
}

VersionSelectWidget::~VersionSelectWidget()
{
}

void VersionSelectWidget::setResizeOn(int column)
{
    listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::ResizeToContents);
    resizeOnColumn = column;
    listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);
}

void VersionSelectWidget::initialize(BaseVersionList *vlist)
{
    m_vlist = vlist;
    m_proxyModel->setSourceModel(vlist);
    listView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);

    if (!m_vlist->isLoaded())
    {
        loadList();
    }
    else
    {
        if (m_proxyModel->rowCount() == 0)
        {
            listView->setEmptyMode(VersionListView::String);
        }
        preselect();
    }
}

void VersionSelectWidget::closeEvent(QCloseEvent * event)
{
    QWidget::closeEvent(event);
}

void VersionSelectWidget::loadList()
{
    auto newTask = m_vlist->getLoadTask();
    if (!newTask)
    {
        return;
    }
    loadTask = newTask.get();
    connect(loadTask, &Task::succeeded, this, &VersionSelectWidget::onTaskSucceeded);
    connect(loadTask, &Task::failed, this, &VersionSelectWidget::onTaskFailed);
    connect(loadTask, &Task::progress, this, &VersionSelectWidget::changeProgress);
    if(!loadTask->isRunning())
    {
        loadTask->start();
    }
    sneakyProgressBar->setHidden(false);
}

void VersionSelectWidget::onTaskSucceeded()
{
    if (m_proxyModel->rowCount() == 0)
    {
        listView->setEmptyMode(VersionListView::String);
    }
    sneakyProgressBar->setHidden(true);
    preselect();
    loadTask = nullptr;
}

void VersionSelectWidget::onTaskFailed(const QString& reason)
{
    CustomMessageBox::selectable(this, tr("Error"), tr("List update failed:\n%1").arg(reason), QMessageBox::Warning)->show();
    onTaskSucceeded();
}

void VersionSelectWidget::changeProgress(qint64 current, qint64 total)
{
    sneakyProgressBar->setMaximum(total);
    sneakyProgressBar->setValue(current);
}

void VersionSelectWidget::currentRowChanged(const QModelIndex& current, const QModelIndex&)
{
    auto variant = m_proxyModel->data(current, BaseVersionList::VersionPointerRole);
    emit selectedVersionChanged(variant.value<BaseVersionPtr>());
}

void VersionSelectWidget::preselect()
{
    if(preselectedAlready)
        return;
    selectCurrent();
    if(preselectedAlready)
        return;
    selectRecommended();
}

void VersionSelectWidget::selectCurrent()
{
    if(m_currentVersion.isEmpty())
    {
        return;
    }
    auto idx = m_proxyModel->getVersion(m_currentVersion);
    if(idx.isValid())
    {
        preselectedAlready = true;
        listView->selectionModel()->setCurrentIndex(idx,QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        listView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}

void VersionSelectWidget::selectRecommended()
{
    auto idx = m_proxyModel->getRecommended();
    if(idx.isValid())
    {
        preselectedAlready = true;
        listView->selectionModel()->setCurrentIndex(idx,QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        listView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}

bool VersionSelectWidget::hasVersions() const
{
    return m_proxyModel->rowCount(QModelIndex()) != 0;
}

BaseVersionPtr VersionSelectWidget::selectedVersion() const
{
    auto currentIndex = listView->selectionModel()->currentIndex();
    auto variant = m_proxyModel->data(currentIndex, BaseVersionList::VersionPointerRole);
    return variant.value<BaseVersionPtr>();
}

void VersionSelectWidget::setExactFilter(BaseVersionList::ModelRoles role, QString filter)
{
    m_proxyModel->setFilter(role, new ExactFilter(filter));
}

void VersionSelectWidget::setFuzzyFilter(BaseVersionList::ModelRoles role, QString filter)
{
    m_proxyModel->setFilter(role, new ContainsFilter(filter));
}

void VersionSelectWidget::setFilter(BaseVersionList::ModelRoles role, Filter *filter)
{
    m_proxyModel->setFilter(role, filter);
}
