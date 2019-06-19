#include "VersionSelectWidget.h"
#include "ui_VersionSelectWidget.h"

#include "BaseVersion.h"
#include "dialogs/CustomMessageBox.h"
#include "VersionProxyModel.h"

VersionSelectWidget::VersionSelectWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::VersionSelectWidget)
{
    setObjectName(QStringLiteral("VersionSelectWidget"));

    m_proxyModel = new VersionProxyModel(this);
    ui->setupUi(this);
    ui->listView->setModel(m_proxyModel);

    ui->loadProgressBar->setVisible(false);

    connect(ui->listView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &VersionSelectWidget::currentRowChanged);
    connect(ui->releasesCheckbox, &QCheckBox::stateChanged, this, &VersionSelectWidget::filterChanged);
    connect(ui->snapshotsCheckbox, &QCheckBox::stateChanged, this, &VersionSelectWidget::filterChanged);
    connect(ui->oldSnapshotsCheckbox, &QCheckBox::stateChanged, this, &VersionSelectWidget::filterChanged);
    connect(ui->betasCheckbox, &QCheckBox::stateChanged, this, &VersionSelectWidget::filterChanged);
    connect(ui->alphasCheckbox, &QCheckBox::stateChanged, this, &VersionSelectWidget::filterChanged);
    connect(ui->refreshButton, &QPushButton::clicked, this, &VersionSelectWidget::loadList);
}

void VersionSelectWidget::setCurrentVersion(const QString& version)
{
    m_proxyModel->setCurrentVersion(version);
    m_currentVersion = version;
}

void VersionSelectWidget::setEmptyString(QString emptyString)
{
    ui->listView->setEmptyString(emptyString);
}

void VersionSelectWidget::setEmptyErrorString(QString emptyErrorString)
{
    ui->listView->setEmptyErrorString(emptyErrorString);
}

VersionSelectWidget::~VersionSelectWidget()
{
    delete ui;
}

void VersionSelectWidget::setResizeOn(int column)
{
    ui->listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::ResizeToContents);
    resizeOnColumn = column;
    ui->listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);
}

void VersionSelectWidget::initialize(BaseVersionList *vlist)
{
    m_vlist = vlist;
    m_proxyModel->setSourceModel(vlist);
    ui->listView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);

    if (!m_vlist->isLoaded())
    {
        loadList();
    }
    else
    {
        if (m_proxyModel->rowCount() == 0)
        {
            ui->listView->setEmptyMode(VersionListView::String);
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
    ui->loadProgressBar->setHidden(false);
}

void VersionSelectWidget::onTaskSucceeded()
{
    if (m_proxyModel->rowCount() == 0)
    {
        ui->listView->setEmptyMode(VersionListView::String);
    }
    ui->loadProgressBar->setHidden(true);
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
    ui->loadProgressBar->setMaximum(total);
    ui->loadProgressBar->setValue(current);
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
        ui->listView->selectionModel()->setCurrentIndex(idx,
                QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        ui->listView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}

void VersionSelectWidget::selectRecommended()
{
    auto idx = m_proxyModel->getRecommended();
    if(idx.isValid())
    {
        preselectedAlready = true;
        ui->listView->selectionModel()->setCurrentIndex(idx,
                QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        ui->listView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}

bool VersionSelectWidget::hasVersions() const
{
    return m_proxyModel->rowCount(QModelIndex()) != 0;
}

BaseVersionPtr VersionSelectWidget::selectedVersion() const
{
    auto currentIndex = ui->listView->selectionModel()->currentIndex();
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

void VersionSelectWidget::filterChanged()
{
    QStringList out;
    if(ui->alphasCheckbox->isChecked())
        out << "(old_alpha)";
    if(ui->betasCheckbox->isChecked())
        out << "(old_beta)";
    if(ui->snapshotsCheckbox->isChecked())
        out << "(snapshot)";
    if(ui->oldSnapshotsCheckbox->isChecked())
        out << "(old_snapshot)";
    if(ui->releasesCheckbox->isChecked())
        out << "(release)";
    auto regexp = out.join('|');
    setFilter(BaseVersionList::TypeRole, new RegexpFilter(regexp, false));
}

void VersionSelectWidget::setFilterBoxVisible(bool visible)
{
    ui->filterBox->setVisible(visible);
}